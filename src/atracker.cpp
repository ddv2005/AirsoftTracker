#include "atracker.h"
#include <CayenneLPP.h>
#include <freertos/FreeRTOS.h>
#include "radio/LoraDevRF95.h"
#include "radio/LoraDevSX1262.h"
#include "dwm1000/DW1000Ng.hpp"
#include "progCommon.h"
#ifdef USE_SCREEN_EVE
#include "atScreenEVE.h"
#else 
#include "atscreendummy.h"
#endif
#include "agd/abcommon.h"


StaticEventGroup_t loraInterruptEventBuffer;
EventGroupHandle_t loraInterruptEvent = xEventGroupCreateStatic( &loraInterruptEventBuffer );
void IRAM_ATTR loraRINT(void);

#ifdef DEBUG_USAGE
class cCPUUsage
{
protected:
    int64_t m_periodStart;
    int64_t m_idleStart;
    int64_t m_workStart;
    int64_t m_idleTime;
    int64_t m_workTime;
    int64_t m_workMaxTime;
    float   m_workUsage;
    float   m_idleUsage;
    int     m_usageCount;
public:
    cCPUUsage()
    {
        m_workMaxTime = m_periodStart = m_idleStart = m_workStart = m_idleTime = m_workTime = 0;
        m_workUsage = 0;
        m_idleUsage = 0;
        m_usageCount = 0;
    }

    void periodStart()
    {
        m_periodStart = esp_timer_get_time();
        m_idleStart = m_workStart = m_idleTime = m_workTime = 0;
    }

    void idleStart()
    {
        m_idleStart = esp_timer_get_time();
    }

    void idleStop()
    {
        m_idleTime += (esp_timer_get_time()-m_idleStart);
        m_idleStart = 0;
    }

    void workStart()
    {
        m_workStart = esp_timer_get_time();  
    }

    void workStop()
    {
        int64_t workTime = (esp_timer_get_time()-m_workStart);
        m_workTime += workTime;
        if(m_workMaxTime<workTime)
            m_workMaxTime = workTime;
        m_workStart = 0;
    }

    void periodStop(const char *msg)
    {
        int64_t period = esp_timer_get_time() - m_periodStart; 
        if(period>0)
        {
            m_workUsage += 100*m_workTime/period;
            m_idleUsage += 100*m_idleTime/period;
            m_usageCount++;
            if(m_usageCount>100)
            {
                DEBUG_MSG("%s work %0.1f%% (%0.2f ms), idle %0.1f%%\n",msg,m_workUsage/m_usageCount, m_workMaxTime/1000.0, m_idleUsage/m_usageCount);
                m_usageCount = 0;
                m_workUsage = m_idleUsage = 0;
                m_workMaxTime = 0;
            }
        }
    }

};
#endif

void printHeap(const char *msg)
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    DEBUG_MSG("%sHeap: %u/%uKB/%uKB\n", msg, info.total_allocated_bytes / 1024, info.total_free_bytes / 1024, info.largest_free_block / 1024);
}

cAirsoftTracker::cAirsoftTracker():m_loraPackets(20),m_loraPacketsToSend(20),m_screen(NULL),m_commands(100),m_networkProcessor(*this,this)
{
    m_core0Thread = NULL;
    m_core0ThreadReady = false;
    m_loraThread = NULL;
    m_imu = NULL;
    m_expander = NULL;
    m_vmotor = NULL;
    memset(m_buttonsTime,0,sizeof(m_buttonsTime));
    m_buttonsIdx[0] = BTN1_PIN;
    m_buttonsIdx[1] = BTN2_PIN;
    m_buttonsIdx[2] = BTN3_PIN;
    m_buttonsIdx[3] = BTN4_PIN;
    m_buttonsIdx[4] = BTN5_PIN;
    m_buttonsIdx[5] = BTN6_PIN;
}

void cAirsoftTracker::onScreenStateChange()
{
    updateSideButtons();
}

bool compare_poi_distance (const sPOI& first, const sPOI& second)
{
  return ( first.distance > second.distance );
}

int32_t tes = 0;
void cAirsoftTracker::mainLoop()
{
    int64_t now = millis();
    atGlobal::loop();
    int powerState = m_power->loop();
    if(powerState & 2) //Longpress
    {
        if(!m_displaySleep)
            m_screen->showShutdownScreen();
        m_config.saveConfig(MAIN_CONFIG_FILENAME);
        m_imu->shutdown();
        DEBUG_MSG("Backup GNSS database\n");
        m_gps.backupDatabase(GNSS_DBD_FILENAME);
        delay(10);
        m_gps.shutdown();
        delay(100);
        DEBUG_MSG("Shutdown\n");
        m_power->shutdown();
    }
    if(powerState & 1)
    {
        m_idleTimer = millis();
    }
    if(isAGDUpdate())
    {
        if((now-m_agdUpdateTimer)>=m_agdNetworkConfig.updateTimeout)
        {
            stopAGDUpdate();
        }
    }
    switch(m_screen->getScreenState())
    {
        case SS_BOOT:
        {
            if(timing::millis() > 1000)
            { 
                m_screen->closeBootScreen();
                m_screen->setScreenState(SS_MAIN);
            }
            break;
        }

        case SS_MAIN:
        {
            m_screen->loop();
            if((now-m_screenTimer)>=50 && !m_displaySleep)
            {
                processTimers(now);
                m_screenTimer = now;
                updateDistances();
                m_screen->beginMainScreen();
                if(m_gpsStatus->getHasLock())
                {
                    int64_t now = millis();

                    int32_t poiCnt = 0;
/*
                    uint16_t radius = m_screen->getRadius();

                    for(auto itr=m_poiMapStatic.begin(); itr!=m_poiMapStatic.end(); itr++)
                    {
                        if(itr->isValidLocation() && itr->distance<=radius)
                        {
                           poiCnt++;
                        }
                    }
*/
                    m_poiMapStatic.sort(compare_poi_distance);


                    for(auto itr=m_poiMapStatic.begin(); itr!=m_poiMapStatic.end(); itr++)
                    {
                        if(itr->isValidLocation())
                        {
                           m_screen->drawPOI((*itr),m_poiMapStatic.size()-poiCnt);
                        }
                        poiCnt++;
                    }

                    cPeers& peers = m_networkProcessor.getPeers();
                    for(auto itr=peers.begin(); itr!=peers.end(); itr++)
                    {
                        if(itr->second.isValidLocation())
                        {
                            if(m_config.maxPeerTimeout==0)
                                m_screen->drawPOI(itr->second);
                            else
                            {
                                int64_t delta = (now-itr->second.lastUpdate)/1000;
                                if(delta<=m_config.maxPeerTimeout)
                                    m_screen->drawPOI(itr->second);
                            }
                        }
                    }

                    for(auto itr=m_poiMap.begin(); itr!=m_poiMap.end(); itr++)
                    {
                        if(itr->second.isValidLocation())
                        {
                            if(m_config.maxPeerTimeout==0)
                                m_screen->drawPOI(itr->second);
                            else
                            {
                                int64_t delta = (now-itr->second.lastUpdate)/1000;
                                if(delta<=m_config.maxPeerTimeout)
                                    m_screen->drawPOI(itr->second);
                            }

                        }
                    }

                    if(m_mapCenterX!=0 && m_mapCenterY!=0)
                    {
                        sPOI self;
                        self.type = POIT_SELF;
                        self.status = (uint8_t)m_playerStatus.status;
                        self.lat = m_gpsStatus->getLatitude()*1e-7;
                        self.lng = m_gpsStatus->getLongitude()*1e-7;
                        self.x = m_gpsStatus->getX();
                        self.y = m_gpsStatus->getY();
                        self.name = "Me";
                        self.symbol = 0;

                        double lat;
                        double lng;
                        double x = m_mapCenterX;
                        double y = m_mapCenterY;
                        convertMeterToDegree(x,y,lng,lat);

                        self.realDistance = distanceBetween(lat,lng,self.lat,self.lng);
                        self.distance = distanceXYBetween(x,y,self.x,self.y);
                        self.courseTo = courseXYTo(x,y,self.x,self.y);
                        self.hacc = m_gpsStatus->getHAcc()/100;
                        m_screen->drawPOI(self);
                    }
                }
                m_screen->drawTimers(m_sideButtonsState==ATSBS_TIMERS);
                m_screen->endMainScreen();
            }
            break;
        }

        case SS_MENU:
        {
            if(!m_displaySleep)
            {
                m_screen->loop();
                if((now-m_screenTimer)>=50)
                {
                    m_screenTimer = now;
                    m_screen->showMenu();
                }
            }
            break;
        }
    }
    if(!m_displaySleep)
    {
        int32_t bk = map(m_ps,0,3000,2,255);
        if(bk>255)
            bk=255;
        m_screen->setBacklight(bk);
    }
    if(!m_displaySleep && ((m_config.screenIdleTimeout>10000 && (now-m_idleTimer)>=m_config.screenIdleTimeout) || m_idleTimer==0))
    {
        m_displaySleep = true;
        m_screen->screenOff();
    }
    if(m_displaySleep && ((now-m_idleTimer)<m_config.screenIdleTimeout || m_config.screenIdleTimeout<10000) && m_idleTimer!=0)
    {
        m_displaySleep = false;
        m_screen->screenOn();
    }
    
    if((now-m_timer1)>=50)
    {
        if(tes++%120 == 0)
        {
            printHeap("");
        }
        m_timer1 = now;
#ifdef HAS_GPS
        m_gps.loop();
#endif  
        processExpander();
        m_vmotor->loop();
        LOCK_DEBUG_PORT();
        while(SERIAL_PROG.available())
        {
            uint8_t byte = SERIAL_PROG.read();
            switch(m_serialSignatureState)
            {
                case 0:
                {
                    if(byte==SERIAL_PROG_MODE_SIG1)
                        m_serialSignatureState++;
                    break;
                }
                case 1:
                {
                    m_serialSignatureState=0;
                    if(byte==SERIAL_PROG_MODE_SIG2)
                    {
                        int64_t serialTime=now;
                        serialTime = now;
                        while(SERIAL_PROG.available()) SERIAL_PROG.read();
                        delay(100);
                        SERIAL_PROG.write(SERIAL_PROG_MODE_RESP);
                        while((millis()-serialTime)<10000)
                        {
                            if(SERIAL_PROG.available())
                            {
                                byte = SERIAL_PROG.read();
                                if(byte==SERIAL_PROG_MODE_RESP)
                                {
                                    SERIAL_PROG.flush();
                                    SERIAL_PROG.updateBaudRate(921600);
                                    if(m_displaySleep)
                                    {
                                        m_displaySleep = false;
                                        m_screen->screenOn();
                                        delay(200);
                                    }
                                    m_screen->showProgramMode();
                                    programMode();
                                    SERIAL_PROG.updateBaudRate(115200);
                                }
                                break;
                            }
                            else
                                delay(1);
                        }
                    }
                    break;
                }
            }
        }
        UNLOCK_DEBUG_PORT();
    }

    m_imu->loop();

    if(!isAGDUpdate())
    {
        processLoraPackets();
    }
    else
    {
        processLoraAGDPackets();
        if((now-m_sideButtonsUpdateTime)>=1000)
        {
            updateSideButtons();
            m_sideButtonsUpdateTime = now;
        }
    }

    m_bleClient.loop();
}

void cAirsoftTracker::processLoraPackets()
{
    m_loraPackets.lock();
    while (m_loraPackets.size() > 0)
    {
        auto packet = m_loraPackets.front();
        m_loraPackets.pop();
        sNetworkPacket netPacket;
        netPacket.data = packet.data;
        netPacket.packetTime = packet.time;
        netPacket.preambleTime = packet.preambleTime;
        m_networkProcessor.processPacket(netPacket,packet.RSSI,packet.SNR);
    }
    m_loraPackets.unlock();
    m_networkProcessor.loop();
}

static SPISettings spiSettings(4000000, MSBFIRST, SPI_MODE0);

void cAirsoftTracker::updateBootScreen()
{
    m_screen->showBootScreen(m_bootMsg1.empty()?NULL:m_bootMsg1.c_str(),m_bootMsg2.empty()?NULL:m_bootMsg2.c_str());    
}

void cAirsoftTracker::init()
{
    auto mainThread = new cThreadFunction(*this, &cAirsoftTracker::mainThread);
    mainThread->start("MainThread", 8192*2, tskIDLE_PRIORITY+1, 1);
}

void cAirsoftTracker::mainThread(cThreadFunction *thread)
{
    mainInit();
    esp_task_wdt_init(APP_WATCHDOG_SECS_WORK, true);
    auto res = esp_task_wdt_add(NULL);
    assert(res == ESP_OK);
#ifdef DEBUG_USAGE
    cCPUUsage usage;
#endif
#ifdef AT_CAN_SLEEP
    int32_t loopCounter = 0;
    pinMode(LED_PIN,OUTPUT);
    digitalWrite(LED_PIN,LOW);
#endif
    while(true)
    {
#ifdef DEBUG_USAGE
        usage.periodStart();
        usage.workStart();
#endif
        unsigned long now = millis();
        mainLoop();
#ifdef DEBUG_USAGE
        usage.workStop();
        usage.idleStart();
#endif
        esp_task_wdt_reset();
        int64_t workTime = ((int64_t)millis())-now;
        if(workTime<0)
            workTime = 0;
        if(workTime<10)
            delay(10-workTime);
        else
            delay(1);

#ifdef AT_CAN_SLEEP
        if(loopCounter++>2)
        {
            bool canSleep = true;
            m_loraPackets.lock();
            canSleep = m_loraPackets.size()==0;
            m_loraPackets.unlock();
            if(canSleep)
            {
                m_loraPacketsToSend.lock();
                canSleep = m_loraPacketsToSend.size()==0;
                m_loraPacketsToSend.unlock();
            }
#ifdef HAS_GPS            
            if(canSleep)
                canSleep = m_gps.canSleep();
#endif                
            if(canSleep)
            {
                m_gps.getSerial().flush();
                digitalWrite(LED_PIN,HIGH);
                detachInterrupt(LORA_IRQ);
                gpio_wakeup_enable((gpio_num_t)LORA_IRQ,GPIO_INTR_HIGH_LEVEL);
                esp_sleep_enable_gpio_wakeup();
                esp_sleep_enable_uart_wakeup(GPS_SERIAL_NUM);
                esp_sleep_enable_timer_wakeup(m_networkProcessor.getTimeslotDuration()*2000/3);
                esp_light_sleep_start();
                digitalWrite(LED_PIN,LOW);
                attachInterrupt(LORA_IRQ, &loraRINT,RISING);
                esp_task_wdt_reset();
                delay(10);
                loopCounter = 0;
            }
        };
#endif    

#ifdef DEBUG_USAGE
        usage.idleStop();
        usage.periodStop("Main Loop usage");
#endif
    }
}

void cAirsoftTracker::mainInit()
{
    DEBUG_MSG("Main Thread on core %d\n", xPortGetCoreID());
    Wire.begin(I2C_SDA, I2C_SCL,400000);
    scanI2Cdevice(Wire);

    atGlobal::init();
    
    setenv("TZ", m_config.timeZone.c_str(), 1);
    tzset();

    DEBUG_MSG("Set TZ %s\n",m_config.timeZone.c_str());

    m_power->gpsOff();
    delay(50);
    m_power->gpsOn();

    sRFModuleConfig loraModule;

#ifdef USE_SX1262    
    loraModule.cs = NSS_GPIO;
    loraModule.irq = SX1262_IRQ;
    loraModule.rst = SX1262_RST;
    loraModule.busy = SX1262_BUSY;
#else
    loraModule.cs = LORA_CS;
    loraModule.irq = LORA_IRQ;
    loraModule.rst = LORA_RST;
    loraModule.busy = 0;
#endif    
    m_expander = new Adafruit_MCP23017();
    m_expander->begin(&Wire);
    for(int i=0; i<16; i++)
        if(i!=VIBRO_PIN && i!=VIBRO_PIN2)
            m_expander->pullUp(i,1);


    m_expander->pinMode(BTN1_PIN,INPUT);
    m_expander->pullUp(BTN1_PIN,1);

    m_expander->pinMode(BTN2_PIN,INPUT);
    m_expander->pullUp(BTN2_PIN,1);

    m_expander->pinMode(BTN3_PIN,INPUT);
    m_expander->pullUp(BTN3_PIN,1);    

    m_expander->pinMode(BTN4_PIN,INPUT);
    m_expander->pullUp(BTN4_PIN,1);    

    m_expander->pinMode(BTN5_PIN,INPUT);
    m_expander->pullUp(BTN5_PIN,1);    

    m_expander->pinMode(BTN6_PIN,INPUT);
    m_expander->pullUp(BTN6_PIN,1);    

    m_expander->pinMode(VIBRO_PIN,OUTPUT);
    m_expander->digitalWrite(VIBRO_PIN,LOW);

    m_expander->pinMode(VIBRO_PIN2,OUTPUT);
    m_expander->digitalWrite(VIBRO_PIN2,LOW);

    m_vmotor = new cVMotor(VIBRO_PIN,VIBRO_PIN2, m_expander);
    vibration();

    DEBUG_MSG("Starting screen\n");
#ifdef USE_SCREEN_EVE
    m_screen  = new cATScreenEVE(*this);
#else    
    m_screen  = new cATScreenDummy(*this);
#endif
    m_expander->pinMode(DW1000_CS,OUTPUT);
    m_expander->digitalWrite(DW1000_CS,HIGH);
    m_vmotor->loop();
#ifndef AD_NOSCREEN
    if(m_maps.getMaps().size())
    {
        cMapsList &maps = m_maps.getMaps();
        DEBUG_MSG("Maps loaded. Count %d\n",maps.size());
        for(auto mitr=maps.begin(); mitr!=maps.end();mitr++)
        {
            cMap &map = *mitr;
            //DEBUG_MSG("  Map %s, zoom levels %d\n",map.getMap().name.c_str(),map.getMapZoomLevels().size());
            DEBUG_MSG("  Map %s\n",map.getMap().name);
        }
    }
    else
        DEBUG_MSG("No maps\n");
#endif
    m_vmotor->loop();
    randomSeed(m_networkConfig.networkId*m_config.id*esp_random());
    m_screen->init(); 
    m_bootMsg1 = "Initializing DWM1000";
    updateBootScreen();          
#if 0
    pinMode(SPI_DISP_CS,OUTPUT);
    digitalWrite(SPI_DISP_CS,HIGH);
    char buffer[128]="";
    DEBUG_MSG("DWM1000\n");
    DW1000Ng::initialize(DW1000_CS,DW1000_IRQ,DW1000_RST,HSPI_HOST,m_expander);

    DW1000Ng::setDeviceAddress(6);
    DW1000Ng::setNetworkId(11);

    DW1000Ng::getPrintableDeviceIdentifier(buffer);
    DEBUG_MSG("DWM1000 OK %s\n",buffer);
    DW1000Ng::getPrintableExtendedUniqueIdentifier(buffer);
    DEBUG_MSG("DWM1000 ID %s\n",buffer);
    DW1000Ng::getPrintableNetworkIdAndShortAddress(buffer);
    DEBUG_MSG("DWM1000 NET %s\n",buffer);
#endif
    
    SPIClass *SPI2 = new SPIClass(VSPI); 

    SPI2->begin(SCK_GPIO, MISO_GPIO, MOSI_GPIO, NSS_GPIO);
    SPI2->setFrequency(4000000);

    loraModule.spi = SPI2;
    loraModule.spiSettings = &spiSettings;

    m_core0Thread = new cThreadFunction(*this, &cAirsoftTracker::core0Thread);
    m_core0Thread->start("Core0", 4096, tskIDLE_PRIORITY+10, 0);

    m_vmotor->loop();
    m_bootMsg1 = "Initializing IMU";
    updateBootScreen();          

    m_imu = new cIMU();

    if(m_imu->setup())
        m_imuStatus->observe(&m_imu->newStatus);
    
    if(m_expander->digitalRead(BTN1_PIN)==LOW)
    {
        m_bootMsg1 = "Calibrating IMU";
        updateBootScreen();      
        m_vmotor->stop();    
        m_imu->calibrate();
    }
    m_vmotor->loop();
    m_bootMsg1 = "Initializing GNSS";
    updateBootScreen();          
#ifdef HAS_GPS
    if(!m_gps.setup())
    {
        for(int i=0; i<5; i++)
        {
            DEBUG_MSG("Restart GPS\n");
            m_bootMsg2 = "Restarting GNSS";
            updateBootScreen();          
            m_power->gpsOff();
            delay(50+10*i);
            m_power->gpsOn();
            delay(1000+100*i);
            if(m_gps.setup())
                break;
        }
    }
    m_gps.startLock();
    m_gpsStatus->observe(&m_gps.newStatus);
#endif
    m_vmotor->stop();
    m_bootMsg1 = "Waiting core 0";
    m_bootMsg2 = "";
    updateBootScreen();          
    while(!m_core0ThreadReady)
    {
        delay(1);
    }

    m_bootMsg1 = "Initializing LORA";
    updateBootScreen();          

#ifdef USE_SX1262
    m_lora = new cLoraDeviceSX1262(loraModule, m_networkConfig.channelConfig);
    int16_t status = m_lora->applyConfig();
    DEBUG_MSG("Lora SX1262 device status %d\n", status);
#else
    m_lora = new cLoraDeviceRF95(loraModule, m_networkConfig.channelConfig);
    int16_t status = m_lora->applyConfig();
    DEBUG_MSG("Lora RF95 device status %d\n", status);
#endif

    if (status == 0)
    {
        m_loraThread = new cThreadFunction(*this, &cAirsoftTracker::loraThread);
        m_loraThread->start("Lora", 4096, tskIDLE_PRIORITY+1, 1);
    }
    m_agdPOI = createPOI();
    m_bootMsg1 = "Initializing BLE";
    updateBootScreen();
    m_bleClient.begin(m_agdPOI);
    m_bootMsg1 = "Initializing complete";
    updateBootScreen();          
    DEBUG_MSG("Setup completed\n");
    m_idleTimer = millis();
    m_displaySleep = false;
    m_serialSignatureState=0;
    while(SERIAL_PROG.available()) SERIAL_PROG.read();
}

volatile bool loraEnableInterrupt;
volatile bool loraReceived;
const TickType_t ticks5ms = 5/portTICK_PERIOD_MS;
const TickType_t ticks1ms = 1/portTICK_PERIOD_MS;

void IRAM_ATTR loraRINT(void)
{
    if (!loraEnableInterrupt)
    {
        return;
    }
    loraReceived = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(loraInterruptEvent,1,&xHigherPriorityTaskWoken);
}

void cAirsoftTracker::loraThread(cThreadFunction *thread)
{
    DEBUG_MSG("Lora Thread on core %d\n", xPortGetCoreID());
    int16_t configID = 0;
    loraEnableInterrupt = true;
    xEventGroupClearBits(loraInterruptEvent,1);
    loraReceived = false;
    m_lora->setDio0Action(loraRINT);
    int status = m_lora->startReceive();
    int64_t lastPreamble = 0;
    DEBUG_MSG("Lora startReceive status %d\n", status);
    while (true)
    {
        if (loraReceived)
        {
            loraReceived = false;
            auto irq = m_lora->getIrqStatus();
            //DEBUG_MSG("%d Lora recv %X\n",millis(),irq);
            if(m_lora->isRxDoneIrq(irq))
            {
                lastPreamble = millis()-m_networkProcessor.getMaxPacketDuration()+(esp_random()%15)+10;
                loraEnableInterrupt = false;
                auto size = m_lora->getPacketLength(true);
                if(size > 0)
                {
                    //DEBUG_MSG("%d Lora has data %d\n",millis(), size);                
                    sLoraPacket packet;
                    packet.data.resize(size);
                    status = m_lora->readData(&packet.data[0], size);
                    if (status == ERR_NONE)
                    {
//                        DEBUG_MSG("%d Lora got packet %d %f\n", millis(),size,m_lora->getSNR());
                        packet.RSSI = m_lora->getRSSI();
                        packet.SNR = m_lora->getSNR();
                        packet.time = millis();
                        packet.preambleTime = lastPreamble;
                        m_loraPackets.push_item(packet);
                    }
                    else
                    {
                        DEBUG_MSG("Lora error read packet %d\n", status);
                    }
                }
                loraEnableInterrupt = true;
                m_lora->startReceive();
            }
            else
            if(m_lora->isPreambleIrq(irq))
            {
                //DEBUG_MSG("%d P\n",millis());
                m_lora->clearPreambleIrq();
                lastPreamble = millis();
            }
        }
        bool hasPacketsToSend = false;
        m_loraPacketsToSend.lock();
        if(m_loraPacketsToSend.size() > 0 && (millis()-lastPreamble)>=m_networkProcessor.getMaxPacketDuration())
        {
            bool lastLoraReceived = loraReceived;
            while (m_loraPacketsToSend.size() > 0)
            {
                auto packet = m_loraPacketsToSend.front();
                m_loraPacketsToSend.pop();
                //DEBUG_MSG("%d Transmit\n",millis());
                //int64_t t = millis();
                auto res = m_lora->transmit(packet.packet.data.data(),packet.packet.data.size());
                //DEBUG_MSG("%d Transmit %d %d %d\n",millis(),(int)packet.packet.data.size(),(int) res, (int)(millis()-t));
            }
            xEventGroupClearBits(loraInterruptEvent,1);
            loraReceived = lastLoraReceived;
            m_lora->startReceive();
        }
        else
        {
            hasPacketsToSend = m_loraPacketsToSend.size();
        }
        
        m_loraPacketsToSend.unlock();

        if(isAGDUpdate())
        {
            if(configID!=1)
            {
                configID = 1;
                m_lora->applyConfig(m_agdNetworkConfig.channelConfig);
                m_lora->startReceive();
                DEBUG_MSG("Lora switch to AGD config\n");
            }
        }
        else
        {
            if(configID!=0)
            {
                configID = 0;
                m_lora->applyConfig(m_networkConfig.channelConfig);
                m_lora->startReceive();
                DEBUG_MSG("Lora switch to main config\n");
            }
        }

        xEventGroupWaitBits(loraInterruptEvent,1,pdTRUE,pdFALSE,hasPacketsToSend?ticks1ms:ticks5ms);
    }
}

void cAirsoftTracker::core0Thread(cThreadFunction *thread)
{
    DEBUG_MSG("Core 0 Thread on core %d\n",xPortGetCoreID());

    m_core0ThreadReady = true;
#ifdef DEBUG_USAGE
    cCPUUsage usage;
#endif
    while (true)
    {
#ifdef DEBUG_USAGE
        usage.periodStart();
        usage.workStart();
#endif
        unsigned long now = millis();

        m_commands.lock();
        while (m_commands.size() > 0)
        {
            sCommand cmd = m_commands.front();
            m_commands.pop();
            m_commands.unlock();
            processCommand(cmd);
            m_commands.lock();
        }
        m_commands.unlock();

#ifdef DEBUG_USAGE
        usage.workStop();
        usage.idleStart();
#endif
        int64_t workTime = ((int64_t)millis())-now;
        if(workTime<0)
            workTime = 0;
        if(workTime<25)
            delay(25-workTime);
        else
            delay(1);
#ifdef DEBUG_USAGE
        usage.idleStop();
        usage.periodStop("Core0 Loop usage");
#endif
    }
}

void cAirsoftTracker::processCommand(const sCommand &cmd)
{
    switch (cmd.id)
    {
    default:
        break;
    }
}

void cAirsoftTracker::processExpander()
{
    int64_t now = millis();
    uint16_t gpioab = m_expander->readGPIOAB();
    //if(gpioab!=m_gpioExpander)
    {
        for(int idx=0; idx<AT_BUTTONS_COUNT; idx++)
        {
            int8_t pin = m_buttonsIdx[idx];
            if(pin>=0)
            {
                uint8_t event = 0;
                uint16_t bit = ((uint16_t)1)<<(uint16_t)pin;
                if((m_gpioExpander & bit) && ((gpioab & bit)==0))
                {
                    event = EV_BTN_ON;
                }
                else 
                    if((m_gpioExpander & bit)==0 && (gpioab & bit))
                        event = EV_BTN_OFF;
                if(event==0 && m_buttonsTime[idx])
                {
                    if((now-m_buttonsTime[idx])>=AT_LONG_CLICK_DURATION)
                        event = EV_BTN_OFF;
                }
                if(event!=0)
                {
                    if(m_screen->getScreenState()==SS_MAIN)
                    {
                        if(m_displaySleep)
                        {
                            if(event==EV_BTN_ON)
                            {
                                m_idleTimer = millis();
                                m_buttonsTime[idx] = 0;
                            }
                        }
                        else
                        {
                            if(event==EV_BTN_ON)
                            {
                                m_idleTimer = millis();
                                m_buttonsTime[idx] = millis();
                            }
                            else
                                if(event==EV_BTN_OFF)
                                {
                                    if(m_buttonsTime[idx])
                                    {
                                        m_idleTimer = millis();
                                        auto duration = (now-m_buttonsTime[idx]);
                                        if(duration>20)
                                            onButtonClick(idx,duration>=AT_LONG_CLICK_DURATION?ATCT_LONG:ATCT_NORMAL);
                                        m_buttonsTime[idx] = 0;
                                    }
                                }
                        }
                    }
                    else
                    {
                        m_idleTimer = millis();
                        m_screen->processEvent(event, idx,0);
                    }
                }
            }
        }
        if(gpioab!=m_gpioExpander)
        {
            DEBUG_MSG("Expander %04X\n",gpioab);
            m_gpioExpander = gpioab;
        }
    }

    uint16_t ps = analogRead(PSENSOR_PIN);
    if(m_ps>0)
        m_ps = (m_ps*10+ps)/11;
    else
        m_ps = ps;
}

void cAirsoftTracker::sendPacket(const sNetworkPacket &packet)
{
    sLoraPacketToSend sp;
    sp.packet = packet;
    m_loraPacketsToSend.push_item(sp);
    //xEventGroupSetBits(loraInterruptEvent,1);
}

void cAirsoftTracker::updateDistances(bool force)
{
    if(m_gpsStatus->getHasLock())
    {
        cPeers& peers = m_networkProcessor.getPeers();
        bool isNewPos = m_gpsStatus->getSequence()!=m_gpsSeq;
        m_gpsSeq = m_gpsStatus->getSequence();
        double lat;// = m_gpsStatus->getLatitude()*1e-7;
        double lng;// = m_gpsStatus->getLongitude()*1e-7;
        double x = m_mapCenterX==0?m_gpsStatus->getX():m_mapCenterX;
        double y = m_mapCenterY==0?m_gpsStatus->getY():m_mapCenterY;
        convertMeterToDegree(x,y,lng,lat);
        for(auto itr=peers.begin(); itr!=peers.end(); itr++)
        {
            if(isNewPos || (itr->second.isValidLocation() && (itr->second.positionChanged || force)))
            {
                cPeer &peer = itr->second;
                peer.realDistance = distanceBetween(lat,lng,peer.lat,peer.lng);
                peer.distance = distanceXYBetween(x,y,peer.x,peer.y);
                peer.courseTo = courseXYTo(x,y,peer.x,peer.y);
                peer.positionChanged = false;
                if(peer.realDistance>100 && itr->second.isValidLocation())
                {
                    peer.retransmitRating = peer.realDistance;
                } 
                else
                    peer.retransmitRating = 0;
            }
        }
        for(auto itr=m_poiMapStatic.begin(); itr!=m_poiMapStatic.end(); itr++)
        {
            if(isNewPos || (itr->isValidLocation() && (itr->positionChanged || force)))
            {
                sPOI &poi = (*itr);
                poi.realDistance = distanceBetween(lat,lng,poi.lat,poi.lng);
                poi.distance = distanceXYBetween(x,y,poi.x,poi.y);
                poi.courseTo = courseXYTo(x,y,poi.x,poi.y);
                poi.positionChanged = false;
            }
        }

        for(auto itr=m_poiMap.begin(); itr!=m_poiMap.end(); itr++)
        {
            if(isNewPos || (itr->second.isValidLocation() && (itr->second.positionChanged || force)))
            {
                sPOI &poi = itr->second;
                poi.realDistance = distanceBetween(lat,lng,poi.lat,poi.lng);
                poi.distance = distanceXYBetween(x,y,poi.x,poi.y);
                poi.courseTo = courseXYTo(x,y,poi.x,poi.y);
                poi.positionChanged = false;
            }
        }
    }
}

void cAirsoftTracker::updateSideButtons()
{
    auto clearButtonFunc = [&](uint8_t idx)
    {
        sSideButton *btn = &m_sideButtons.getButtons()[idx];
        btn->action = BA_NONE;
        btn->actionLong = BA_NONE;
        btn->caption = "";
    };

    if(m_screen->getScreenState()==SS_MENU)
    {
        
        sSideButton *btn = &m_sideButtons.getButtons()[2];
        btn->action = BA_NONE;
        btn->actionLong = BA_NONE;
        btn->caption = "<BACK";

        btn = &m_sideButtons.getButtons()[0];
        btn->action = BA_NONE;
        btn->actionLong = BA_NONE;
        btn->caption = "UP";

        btn = &m_sideButtons.getButtons()[1];
        btn->action = BA_NONE;
        btn->actionLong = BA_NONE;
        btn->caption = "DOWN";

        btn = &m_sideButtons.getButtons()[3];
        btn->action = BA_NONE;
        btn->actionLong = BA_NONE;
        btn->caption = "PREV";

        btn = &m_sideButtons.getButtons()[4];
        btn->action = BA_NONE;
        btn->actionLong = BA_NONE;
        btn->caption = "OK";

        btn = &m_sideButtons.getButtons()[5];
        btn->action = BA_NONE;
        btn->actionLong = BA_NONE;
        btn->caption = "NEXT";
    }
    else
    switch(m_sideButtonsState)
    {
        case ATSBS_MAIN:
        {
            char buffer[32];
            sSideButton *btn = &m_sideButtons.getButtons()[3];
            btn->actionLong = BA_STATUS;
            btn->action = BA_NONE;
            if(m_playerStatus.status==PS_NORMAL)
                btn->caption = ATS_BTN_DEAD;
            else
                btn->caption = ATS_BTN_ALIVE;

            btn = &m_sideButtons.getButtons()[0];
            btn->action = BA_RADIUS;
            btn->actionLong = BA_MAP;
            btn->caption = ATS_MRADIUS;

            btn = &m_sideButtons.getButtons()[1];
            btn->action = BA_MENU;
            btn->actionLong = BA_NONE;
            btn->caption = ATS_MENU;

        #ifndef AD_BLE_NO_CLIENT
            btn = &m_sideButtons.getButtons()[2];
            btn->action = BA_BLE;
            btn->actionLong = BA_AGD_UPDATE;
            if(m_agdUpdateTimer>0)
                snprintf(buffer,sizeof(buffer),"%s/AGD UPD (%ds)",m_bleClient.getEnableScan()?"D BLE":"E BLE",(int)(millis()-m_agdUpdateTimer)/1000);
            else
                snprintf(buffer,sizeof(buffer),"%s/AGD UPD",m_bleClient.getEnableScan()?"D BLE":"E BLE");
            btn->caption = buffer;
        #endif    

            btn = &m_sideButtons.getButtons()[4];
            btn->action = BA_TIMERS;
            btn->actionLong = BA_TIMERS_RST;
            btn->caption = "Timers/RST";

            btn = &m_sideButtons.getButtons()[5];
            btn->action = BA_NONE;
            btn->actionLong = BA_SLEEP;
            btn->caption = "";

            break;
        }

        case ATSBS_MAP:
        {
            sSideButton *btn = &m_sideButtons.getButtons()[0];
            btn->action = BA_RADIUS;
            btn->actionLong = BA_MAPRESET;
            btn->caption = "RADIUS/RST";

            btn = &m_sideButtons.getButtons()[1];
            btn->action = BA_MAPLEFT;
            btn->actionLong = BA_NONE;
            btn->caption = "LEFT";

            btn = &m_sideButtons.getButtons()[2];
            btn->action = BA_MAPEXIT;
            btn->actionLong = BA_NONE;
            btn->caption = "<BACK";

            btn = &m_sideButtons.getButtons()[3];
            btn->action = BA_MAPUP;
            btn->actionLong = BA_NONE;
            btn->caption = "UP";

            btn = &m_sideButtons.getButtons()[4];
            btn->action = BA_MAPRIGHT;
            btn->actionLong = BA_NONE;
            btn->caption = "RIGHT";

            btn = &m_sideButtons.getButtons()[5];
            btn->action = BA_MAPDOWN;
            btn->actionLong = BA_NONE;
            btn->caption = "DOWN";
            break;
        }

        case ATSBS_TIMERS:
        {
            char buffer[32];
            sSideButton *btn;
            if(m_timers[2]->isEnabled())
            {
                btn = &m_sideButtons.getButtons()[0];
                btn->action = BA_TIMER;
                btn->actionLong = BA_TIMER;
                snprintf(buffer,sizeof(buffer),"%s T3/RST",m_timers[2]->isStarted()?(m_timers[2]->isPaused()?"R":"P"):"S");
                btn->caption = buffer;
            }
            else
                clearButtonFunc(0);

            if(m_timersChains.front()->isEnabled())
            {
                btn = &m_sideButtons.getButtons()[1];
                btn->action = BA_TIMER;
                btn->actionLong = BA_TIMER;
                snprintf(buffer,sizeof(buffer),"%s T1->T2/RST",m_timersChains.front()->isStarted()?(m_timersChains.front()->isPaused()?"R":"P"):"S");
                btn->caption = buffer;
            }
            else
                clearButtonFunc(1);

            btn = &m_sideButtons.getButtons()[2];
            btn->action = BA_EXIT;
            btn->actionLong = BA_TIMER;
            btn->caption = "<BACK";

            if(m_timers[3]->isEnabled())
            {
                btn = &m_sideButtons.getButtons()[3];
                btn->action = BA_TIMER;
                btn->actionLong = BA_TIMER;
                snprintf(buffer,sizeof(buffer),"%s T4/RST",m_timers[3]->isStarted()?(m_timers[3]->isPaused()?"R":"P"):"S");
                btn->caption = buffer;
            }
            else
                clearButtonFunc(3);

            clearButtonFunc(4);
            clearButtonFunc(5);
            break;
        }

    }
}

inline static void calcDirectionPoint(double angle, int32_t radius, double &x,double &y)
{
    if(angle<0)
        angle += 2*PI;
    else
    if(angle>2*PI)
        angle -= 2*PI;
    x = radius*cos(angle);
    y = radius*sin(angle);
}

void cAirsoftTracker::onButtonClick(uint8_t idx,uint8_t type)
{
    uint8_t action = 0;
    if(idx<m_sideButtons.getButtonsCount())
    {
        switch(type)
        {
            case ATCT_NORMAL:
                action = m_sideButtons.getButtons()[idx].action;
                break;
            case ATCT_LONG:
                action = m_sideButtons.getButtons()[idx].actionLong;
                break;
        }
    }
    DEBUG_MSG("On button Click %d, action %d\n",idx,action);
    switch(action)
    {
        case BA_TIMER:
        {
            int64_t now = millis();            
            if(idx==1) // Timers chain
            {
                cTimersChain *tc = m_timersChains.front();
                if(type==ATCT_NORMAL)
                {
                    if(tc->isStarted())
                    {
                        if(tc->isPaused())
                            tc->resume(now);
                        else
                            tc->pause(now);
                    }
                    else
                        tc->start(now);
                }
                else
                    tc->reset();

            }
            else
            {
                cTimer *t;
                if(idx==0)
                    t = m_timers[2];
                else
                    t = m_timers[3];
                if(type==ATCT_NORMAL)
                {
                    if(t->isStarted())
                    {
                        if(t->isPaused())
                            t->resume(now);
                        else
                            t->pause(now);
                    }
                    else
                        t->start(now);
                }
                else
                    t->reset();
            }
            updateSideButtons();
            break;
        }
        case BA_STATUS:
        {
            if(m_playerStatus.status==PS_NORMAL)
            {
                m_playerStatus.status = PS_DEAD;
            }
            else
            {
                m_playerStatus.status = PS_NORMAL;
            }
            updateSideButtons();
            break;
        }

        case BA_MENU:
        {
            m_screen->setScreenState(SS_MENU);
            break;
        }

        case BA_AGD_UPDATE:
        {
            if(isAGDUpdate())
                stopAGDUpdate();
            else
                startAGDUpdate();
            updateSideButtons();
            break;
        }

        case BA_BLE:
        {
#ifndef AD_BLE_NO_CLIENT
            m_bleClient.enableScan(!m_bleClient.getEnableScan());
            updateSideButtons();
#endif
            break;
        }

        case BA_MAP:
        {
            m_sideButtonsState = ATSBS_MAP;
            updateSideButtons();
            break;
        }

        case BA_SLEEP:
        {
            m_idleTimer = 0;
            break;
        }

        case BA_TIMERS:
        {
            m_sideButtonsState = ATSBS_TIMERS;
            updateSideButtons();
            break;
        }

        case BA_TIMERS_RST:
        {
            for(auto itr = m_timersChains.begin(); itr!=m_timersChains.end(); itr++)
                (*itr)->reset();

            for(auto itr = m_timers.begin(); itr!=m_timers.end(); itr++)
                itr->second->reset();
            break;
        }

        case BA_MAPLEFT:
        case BA_MAPRIGHT:
        case BA_MAPUP:
        case BA_MAPDOWN:
        {
            double x,y;
            double direction = getDirection();
            switch(action)
            {
                case BA_MAPUP: direction -= PI/2; break;
                case BA_MAPDOWN: direction += PI/2; break;
                case BA_MAPLEFT: break;
                case BA_MAPRIGHT: direction -= PI; break;
            }
            calcDirectionPoint(direction,getConfig().radius/10,x,y);
            if(m_mapCenterX==0 || m_mapCenterY==0)
            {
                m_mapCenterX = m_gpsStatus->getX();
                m_mapCenterY = m_gpsStatus->getY();
            }
            m_mapCenterX += x;
            m_mapCenterY += y;
            updateDistances(true);
            break;
        }

        case BA_MAPRESET:
        {
            m_mapCenterX = m_mapCenterY = 0;
            updateDistances(true);
            break;
        }

        case BA_MAPEXIT:
        {
            m_mapCenterY = m_mapCenterX = 0;
            m_sideButtonsState = ATSBS_MAIN;
            updateDistances(true);
            updateSideButtons();
            break;
        }

        case BA_EXIT:
        {
            m_sideButtonsState = ATSBS_MAIN;
            updateSideButtons();
            break;
        }

        case BA_RADIUS:
        {
            switch(m_config.radius)
            {
                case 250:
                {
                    m_config.radius = 500;
                    break;
                }

                case 500:
                {
                    m_config.radius = 1000;
                    break;
                }

                case 1000:
                {
                    m_config.radius = 25;
                    break;
                }


                case 25:
                {
                    m_config.radius = 50;
                    break;
                }

                case 50:
                {
                    m_config.radius = 100;
                    break;
                }

                case 100:
                {
                    m_config.radius = 150;
                    break;
                }

                case 150:
                {
                    m_config.radius = 250;
                    break;
                }
            }
            updateSideButtons();
            break;
        }       
    }
}

void cAirsoftTracker::vibration()
{
    if(m_config.useVibration)
        m_vmotor->vibrate(m_config.vibrationTime);
}

#define prgSendPacket(P,V) SERIAL_PROG.write(newPacketBuffer,serialParser.create_packet(newPacketBuffer,MAX_SERIAL_PACKET_SIZE,P,&V,sizeof(V)))
void cAirsoftTracker::programMode()
{
    byte_t *newPacketBuffer = (byte_t *)malloc(MAX_SERIAL_PACKET_SIZE+4);
    bool needReset = false;
    int64_t lastTime = millis();
    comm_protocol_parser_c serialParser(cp_prefix_t(SERIAL_APP_1, SERIAL_APP_2, SERIAL_APP_3, SERIAL_APP_4),MAX_SERIAL_PACKET_SIZE);
    byte_t *packetBuffer = serialParser.get_packet_buffer();
    FILE *metaFile = NULL;
    FILE *dataFile = NULL;
    vTaskPrioritySet( NULL, configMAX_PRIORITIES - 5 );
    while((millis()-lastTime)<10000)
    {
        //bool debug = true;//(millis()-lastTime)>10000;
        while(SERIAL_PROG.available())
        {
            byte_t incomingByte = SERIAL_PROG.read();
/*            
            if(debug)
            {
                SERIAL_PROG.write('Z');
                SERIAL_PROG.write(incomingByte);
                SERIAL_PROG.flush();
            }
*/            
            //lastTime = millis();
            int8_t res = serialParser.post_data(&incomingByte,sizeof(byte_t));
  /*          
            if(debug)
            {
                SERIAL_PROG.write(res);
                SERIAL_PROG.flush();
            }
*/            
            if(res== CP_RESULT_PACKET_READY)
            {
                esp_task_wdt_reset();
                lastTime = millis();
                uint16_t packetSize=serialParser.get_packet_size();
                if(packetSize>=1)
                {
                    uint8_t command = packetBuffer[0];
                    switch(command)
                    {
                        case PMC_SCREEN_FLASH_META_START:
                        {
                            metaFile = fopen(SCREEN_FLASH_META_BINARY_FILE,"wb+");
                            byte_t result = 0;
                            if(metaFile)
                                prgSendPacket(PMC_SCREEN_FLASH_ACK,result);
                            else
                                prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                            break;
                        }

                        case PMC_SCREEN_FLASH_META_END:
                        {
                            if(metaFile)
                            {
                                fflush(metaFile);
                                fclose(metaFile);
                                metaFile = NULL;
                            }
                            byte_t result = 0;
                            prgSendPacket(PMC_SCREEN_FLASH_ACK,result);       
                            break;                     
                        }

                        case PMC_SCREEN_FLASH_META_DATA:
                        {
                            byte_t result = 0;
                            if(metaFile)
                            {
                                fwrite(&packetBuffer[1],packetSize-1,1,metaFile);
                                prgSendPacket(PMC_SCREEN_FLASH_ACK,result);
                            }
                            else
                            {
                                prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                            }
                            break;
                        }

                        case PMC_SCREEN_FLASH_FILE_START:
                        {
                            char fileName[32];
                            packetBuffer[packetSize-1] = 0;
                            snprintf(fileName,sizeof(fileName),"%s%s",SCREEN_FLASH_META_FILE_PATH,&packetBuffer[1]);
                            dataFile = fopen(fileName,"wb+");
                            byte_t result = 0;
                            if(dataFile)
                                prgSendPacket(PMC_SCREEN_FLASH_ACK,result);
                            else
                                prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                            break;
                        }

                        case PMC_SCREEN_FLASH_FILE_END:
                        {
                            if(dataFile)
                            {
                                fflush(dataFile);
                                fclose(dataFile);
                                dataFile = NULL;
                            }
                            byte_t result = 0;
                            prgSendPacket(PMC_SCREEN_FLASH_ACK,result);       
                            break;                     
                        }

                        case PMC_SCREEN_FLASH_FILE_DATA:
                        {
                            byte_t result = 0;
                            if(dataFile)
                            {
                                fwrite(&packetBuffer[1],packetSize-1,1,dataFile);
                                prgSendPacket(PMC_SCREEN_FLASH_ACK,result);
                            }
                            else
                            {
                                prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                            }
                            break;
                        }

                        case PMC_SCREEN_FLASH_START:
                        {
                            uint32_t result = m_screen->flashProgrammStart();
                            prgSendPacket(PMC_SCREEN_FLASH_START_RESPONSE,result);
                            break;
                        }
                        case PMC_SCREEN_FLASH_END:
                        {
                            m_screen->flashProgrammEnd();
                            int32_t result = 0;
                            prgSendPacket(PMC_SCREEN_FLASH_ACK,result);
                            break;
                        }
                        case PMC_SCREEN_FLASH_WRITE_COMPRESS:
                        case PMC_SCREEN_FLASH_WRITE:
                        {
                            needReset = true;
                            int32_t result = 0;
                            sPMCFlashBlock *fb = (sPMCFlashBlock*)&packetBuffer[1];
                            uint8_t *data = &packetBuffer[1+sizeof(sPMCFlashBlock)];
                            if(fb && data && fb->size==(packetSize-1-sizeof(sPMCFlashBlock)))
                            {
                                if(command==PMC_SCREEN_FLASH_WRITE_COMPRESS)
                                {
                                    for(int i=0; i<fb->size; i++)
                                    {
                                        if(i%2==0)
                                            data[i] ^= 0xAA;
                                        else
                                            data[i] ^= 0x55;
                                    }
                                }
                                m_screen->flashWrite(fb->address,fb->size,data);
                                delay(1);
                                prgSendPacket(PMC_SCREEN_FLASH_ACK,result);
                            }
                            else
                            {
                                result = -1;
                                prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                            }
                            
                            break;
                        }
                        case PMC_SCREEN_FLASH_READ:
                        {
                            int32_t result = 0;
                            sPMCFlashBlock fb = *(sPMCFlashBlock*)&packetBuffer[1];
                            if(fb.size<=MAX_SERIAL_PACKET_SIZE)
                            {
                                m_screen->flashRead(fb.address,fb.size,packetBuffer);
                                SERIAL_PROG.write(newPacketBuffer,serialParser.create_packet(newPacketBuffer,MAX_SERIAL_PACKET_SIZE,PMC_SCREEN_FLASH_READ_RESPONSE,packetBuffer,fb.size));
                            }
                            else
                            {
                                result = -1;
                                prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                            }
                            
                            break;
                        }

                        case PMC_SCREEN_FLASH_EXIT:
                        {
                            lastTime = 0;
                            break;
                        }

                        default:
                        {
                            int32_t result = -100;
                            prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                            break;
                        }
                    }
                }
                else
                {
                    int32_t result = -1;
                    prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                }
                serialParser.reset();
                SERIAL_PROG.flush();
            }
            else
            if(res<=-2)
            {
                int32_t result = res;
                prgSendPacket(PMC_SCREEN_FLASH_ERR,result);
                SERIAL_PROG.flush();
            }
        }
        esp_task_wdt_reset();
        yield();
    }
    if(metaFile)
        fclose(metaFile);
    if(needReset)
    {
        delay(1000);
        ESP.restart();
        delay(1000);
    }
    vTaskPrioritySet( NULL, tskIDLE_PRIORITY+1);
    free(newPacketBuffer);
}

struct sLoraPacketHeader
{
    uint8_t tag;
    uint8_t src;
    uint8_t dst;
} __attribute__((packed));

void cAirsoftTracker::processLoraAGDPackets()
{
    bool hasUpdate = false;
    m_loraPackets.lock();
    while (m_loraPackets.size() > 0)
    {
        auto packet = m_loraPackets.front();
        m_loraPackets.pop();

        if (packet.data.size() > sizeof(sLoraPacketHeader))
        {
            sLoraPacketHeader *header = (sLoraPacketHeader *)&packet.data[0];
            uint8_t *data = &packet.data[sizeof(sLoraPacketHeader)];
            if (header->tag == LORA_TAG)
            {
                sPOI poi;
                int32_t time1 = 0;
                int32_t time2 = 0;
                int32_t timeMode1 = 0;
                int32_t timeMode2 = 0;


                poi.lastUpdate = packet.time;
                poi.RSSI = packet.RSSI;
                poi.SNR = packet.SNR;
                poi.symbol = 'D';
                poi.type = POIT_DYNAMIC;
                poi.positionChanged = true;

                DynamicJsonDocument jsonBuffer(1024);
                JsonArray root = jsonBuffer.to<JsonArray>();
                CayenneLPP lpp(0);
                lpp.decode(data, packet.data.size() - sizeof(sLoraPacketHeader), root);
                for (const JsonVariant &value : root)
                {
                    uint16_t ch = value["channel"];
                    switch(ch)
                    {
                        case LSID_HACC:
                        {
                            poi.hacc = value.as<JsonObject>()["value"];
                            break;
                        }

                        case LSID_GPS:
                        {
                            JsonVariant gvalue = value.as<JsonObject>()["value"];
                            JsonObject gavalue = gvalue.as<JsonObject>();
                            poi.lng = gavalue["longitude"];
                            poi.lat = gavalue["latitude"];
                            poi.alt = gavalue["altitude"];
                            break;
                        }

                        case LSID_TIMER1:
                        {
                            time1 = value.as<JsonObject>()["value"];
                            break;
                        }

                        case LSID_TIMER2:
                        {
                            time2 = value.as<JsonObject>()["value"];
                            break;
                        }

                        case LSID_TIMERS_MODE:
                        {
                             int v = value.as<JsonObject>()["value"];
                             timeMode1 = v & 0xF;
                             timeMode2 = (v & 0xF0)>>4;
                            break;
                        }

                    }
                }
                poi.updateXY();
                char buffer[32];
                snprintf(buffer,sizeof(buffer),"R %02d:%02d%s,B %02d:%02d%s",time1/60,time1 % 60,timeMode1==1?"*":"",time2/60,time2 % 60,timeMode2==1?"*":"");
                poi.name = buffer;
                setPOI(m_agdPOI,poi);
                hasUpdate = true;
            }
        }
    }
    m_loraPackets.unlock();
    if(hasUpdate)
        stopAGDUpdate();
}

void cAirsoftTracker::startAGDUpdate()
{
    processLoraPackets();
    m_agdUpdateTimer = millis();
}

void cAirsoftTracker::stopAGDUpdate()
{
    m_loraPackets.lock();
    while(!m_loraPackets.empty())
        m_loraPackets.pop();
    m_loraPackets.unlock();
    m_agdUpdateTimer = 0;
    updateSideButtons();
}
