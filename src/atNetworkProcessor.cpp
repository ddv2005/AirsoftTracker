#include "atNetworkProcessor.h"
#include "mbedtls/md.h"
#include <esp_crc.h>
#include <climits>
#include <algorithm>
cPeer::cPeer()
{
    m_id = 0;
    m_lastPacketTime = 0;
    m_lastRequestInfo = 0;
    m_lastRetransmit = 0;
}

void cPeer::init(const char *key, uint8_t  size, uint8_t id,  uint8_t netId)
{
    memset(m_timeslots,0,sizeof(m_timeslots));
    type = POIT_PEER;
    m_id = id;
    if(m_id>128)
        type = POIT_ENEMY;
    m_cryptoEngine.initWithKey(key,size,id,netId);
}

bool cPeer::needRequestInfo(int64_t now, int64_t interval)
{
    if(!name.empty())
        return false;
    return (now-m_lastRequestInfo)>=interval;
}

uint16_t cPeer::getTimeslotOffset(uint8_t timeslot, uint16_t timeslotDuration)
{
    return timeslot*timeslotDuration;
}

void cPeer::updateTimeslot(int64_t packetTime, uint16_t timeslotDuration, uint8_t timeslot, uint8_t timeslotNext, int64_t now)
{
    if(packetTime>0)
        m_peerTimeOffset = (packetTime/TIMESLOTS_PERIOD)*TIMESLOTS_PERIOD+getTimeslotOffset(timeslot,timeslotDuration)-packetTime-1;
    if(m_peerTimeOffset<0)
        m_peerTimeOffset += TIMESLOTS_PERIOD;
    updateTimeslot(timeslot,now);
    if(timeslotNext<TIMESLOTS_PER_PEER_MAX)
        updateTimeslot(timeslotNext,now);
}

void cPeer::updateTimeslot(uint8_t timeslot,int64_t now)
{
    if(timeslot>=TIMESLOTS_MAX)
        return;
    bool found = false;
    for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
    {
        if(m_timeslots[i].inUse && m_timeslots[i].timeslot==timeslot)
        {
            found = true;
            m_timeslots[i].lastTime = now;
            break;
        }
    }
    if(!found)
    {
        int idx = -1;
        int64_t minTime = m_timeslots[0].lastTime;
        for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
        {
            if(!m_timeslots[i].inUse)
            {
                found = true;
                m_timeslots[i].inUse = 1;
                m_timeslots[i].timeslot = timeslot;
                m_timeslots[i].lastTime = now;
                break;
            }
            else
            {
                if(m_timeslots[i].lastTime<minTime)
                    idx = i;
            }
        }
        if(!found)
        {
            m_timeslots[idx].inUse = 1;
            m_timeslots[idx].timeslot = timeslot;
            m_timeslots[idx].lastTime = now;            
        }
    }
    for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
    {
        if(m_timeslots[i].inUse && (now-m_timeslots[i].lastTime)>=TIMESLOT_TIMEOUT)
        {
            m_timeslots[i].inUse = 0;
        }
    }
}

cCryptoEngine::cCryptoEngine()
{
    memset(m_key,0x55,sizeof(m_key));
}

void cCryptoEngine::initWithKey(const char *key, uint8_t  size, uint8_t id,  uint8_t netId)
{
    memset(m_key,0x55,sizeof(m_key));
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *) &netId, 1);
    mbedtls_md_update(&ctx, (const unsigned char *) key, size);
    mbedtls_md_update(&ctx, (const unsigned char *) &id, 1);
    mbedtls_md_finish(&ctx, m_key);
    mbedtls_md_free(&ctx);
}

void cCryptoEngine::encode(uint8_t *data, uint16_t size)
{
    uint8_t keyPosition = 0;
    for(int i=0; i<size; i++)
    {
        data[i] = data[i]^m_key[keyPosition];
        data[i] = (data[i] + m_key[(keyPosition+1) % CK_SIZE]) & 0xFF;
        data[i] = data[i]^m_key[(keyPosition+2) % CK_SIZE];
        keyPosition++;
        keyPosition = keyPosition % CK_SIZE;
    }
}

bool cCryptoEngine::decode(uint8_t *data, uint16_t size)
{
    uint8_t keyPosition = 0;
    for(int i=0; i<size; i++)
    {
        data[i] = data[i]^m_key[(keyPosition+2) % CK_SIZE];
        data[i] = (data[i] - m_key[(keyPosition+1) % CK_SIZE]) & 0xFF;
        data[i] = data[i]^m_key[keyPosition];        
        keyPosition++;
        keyPosition = keyPosition % CK_SIZE;
    }
    return true;
}

cNetworkProcessor::cNetworkProcessor(atGlobal &global,cNetworkProcessorInterface *interface):m_global(global), m_interface(interface)
{
    memset(m_timeslots,0,sizeof(m_timeslots));
    memset(m_timeslotsMap,NETWORK_NODE_ID_MAX,sizeof(m_timeslotsMap));
    m_infoRequested = true;
    m_bufferSize = 0;
    m_reportInterval = 500;
    m_lastReportTime = 0;
    m_lastInfoTime = 0;
    updateConfig();
}

void cNetworkProcessor::processPacketItem(cPeer *peer, const sNetworkPackeItemHeader *header,const uint8_t *data, uint16_t size)
{
    //DEBUG_MSG("Packet from %02X, type %d\n",peer->getId(),header->type);
    switch(header->type)
    {
        case PT_REPORT_REMOTE_D1:
        case PT_REPORT_REMOTE:
        {
            if(size>=sizeof(sNpReportRemoteHeader))
            {
                const sNpReportRemoteHeader *rheader = (sNpReportRemoteHeader *)data;
                if(rheader->src!=m_global.getConfig().id)
                {
                    cPeer *remotePeer = NULL;
                    auto itr = m_peers.find(rheader->src);
                    if(itr!=m_peers.end())
                    {
                        remotePeer = & itr->second;
                    }
                    else
                    {
                        atNetworkConfig &netConfig = m_global.getNetworkConfig();
                        cPeer newPeer;
                        m_peers[rheader->src] = newPeer;
                        remotePeer = & m_peers[rheader->src];
                        remotePeer->init(netConfig.password.c_str(),netConfig.password.length(),rheader->src,netConfig.networkId);
                    }
                    remotePeer->RSSI = 0;
                    remotePeer->SNR = 0;
                    std::list<int> tsList;
                    if(rheader->ts.d4.flag) //D3
                    {
                        if(rheader->ts.d3.t0<(TIMESLOTS_MAX-1))
                        {
                            int lastTS = rheader->ts.d3.t0;
                            tsList.push_back(lastTS);
                            for(int i=1; i<=rheader->ts.d3.size(); i++)
                            {
                                int ts = rheader->ts.d3.get(i);
                                if(ts>0)
                                {
                                    lastTS += ts;
                                    tsList.push_back(lastTS);
                                }
                                else
                                    break;
                            }
                        }
                    }
                    else //D4
                    {
                        if(rheader->ts.d4.t0<(TIMESLOTS_MAX-1))
                        {
                            int lastTS = rheader->ts.d4.t0;
                            tsList.push_back(lastTS);
                            for(int i=1; i<=rheader->ts.d4.size(); i++)
                            {
                                int ts = rheader->ts.d4.get(i);
                                if(ts>0)
                                {
                                    lastTS += ts;
                                    tsList.push_back(lastTS);
                                }
                                else
                                    break;
                            }
                        }
                    }
                    if(tsList.size()>0)
                    {
                        int64_t now = millis();
                        //DEBUG_MSG("Remote Update %02X, Timeslots",rheader->src);
                        for(auto itr=tsList.begin(); itr!=tsList.end(); itr++)
                        {
                            remotePeer->updateTimeslot(*itr,now);
                            //DEBUG_MSG(" %d",(int)*itr);
                        }
                        //DEBUG_MSG("\n");
                        if(rheader->src<m_global.getConfig().id)
                        {
                            rebuildTimeslotsMap();
                            for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
                            {
                                if(m_timeslots[i].inUse && std::find(tsList.begin(),tsList.end(),m_timeslots[i].timeslot)!=tsList.end())
                                {
                                    m_timeslots[i].timeslot = findTimeslotAround(m_timeslots[i].timeslot);
                                    if(m_timeslots[i].timeslot==TIMESLOTS_MAX)
                                        m_timeslots[i].inUse = 0;
                                }
                            }
                        }
                    }
                    data += sizeof(sNpReportRemoteHeader);
                    size += sizeof(sNpReportRemoteHeader);
                    if(header->type==PT_REPORT_REMOTE_D1)
                    {
                        if(size>=sizeof(sNpReportItemD1) && m_lastPeerReport)
                        {
                            sNpReportItemD1 *reportD1 = (sNpReportItemD1 *)data;
                            sNpReportItem report;
                            report.hacc = reportD1->hacc;
                            report.heading = reportD1->heading;
                            report.status = reportD1->status;
                            report.options = reportD1->options;
                            report.lat = reportD1->lat+m_lastPeerReport->lat;
                            report.lng = reportD1->lng+m_lastPeerReport->lng;
                            report.alt = reportD1->alt+m_lastPeerReport->alt;
                            remotePeer->updateReport(&report);
                            DEBUG_MSG("Get %02X position %f, %f\n",remotePeer->getId(),remotePeer->getReport().lat*1e-6,remotePeer->getReport().lng*1e-6);
                        }
                    }
                    else
                    {
                        if(size>=sizeof(sNpReportItem))
                        {
                            sNpReportItem * report = (sNpReportItem *)data;
                            remotePeer->updateReport(report);
                        }
                    }
                    remotePeer->updateLastPacketTime();                
                    remotePeer->updateLastRetransmit();
                }
            }
            break;
        }

        case PT_REPORT:
        {
            if(size>=sizeof(sNpReportItem))
            {
                m_lastPeerReport = (sNpReportItem *)data;
                peer->updateReport(m_lastPeerReport);
                //DEBUG_MSG("Get %02X position %f, %f\n",(int)m_lastPeerReport->getId(),m_lastPeerReport->getReport().lat*1e-6,m_lastPeerReport->getReport().lng*1e-6);
            }
            else
                DEBUG_MSG("Wrong packet size %d %d\n",(int)sizeof(sNpReportItem),size);

            break;
        }

        case PT_INFO_REQUEST:
        {
            if(size>=sizeof(sNpInfoRequest))
            {
                sNpInfoRequest *ir = (sNpInfoRequest *)data;
                if(size==(sizeof(sNpInfoRequest)+ir->count*sizeof(sNpInfoRequestItem)))
                {
                    uint8_t id = m_global.getConfig().id;
                    data += sizeof(sNpInfoRequest);
                    for(int i=0; i<ir->count; i++)
                    {
                        sNpInfoRequestItem *iri = (sNpInfoRequestItem *)data;
                        if(iri->id==id)
                        {
                            DEBUG_MSG("Request info\n");
                            m_infoRequested = true;
                            break;
                        }
                        data += sizeof(sNpInfoRequestItem);
                    }
                }
                else
                    DEBUG_MSG("Wrong packet size %d %d\n",(int)(sizeof(sNpInfoRequest)+ir->count*sizeof(sNpInfoRequestItem)),size);
            }
            break;
        }

        case PT_INFO_RESPONSE:
        {
            if(size==sizeof(sNpInfoResponse))
            {
                peer->updateInfo((sNpInfoResponse *)data);
            }
            else
            {
                DEBUG_MSG("Wrong packet size %d %d\n",(int)sizeof(sNpInfoResponse),size);
            }
            
            break;
        }
    }
}

void cNetworkProcessor::processPacket(const sNetworkPacket &packet, float RSSI, float SNR)
{
    const sNetworkPacketHeader *ph;
    m_lastPeerReport = NULL;
    if(packet.data.size()>=sizeof(sNetworkPacketHeader))
    {
        uint8_t *data = (uint8_t *)packet.data.data();
        ph = (const sNetworkPacketHeader *)data;
        if(ph->netId==(m_global.getNetworkConfig().networkId&0xF) && ph->src!=m_global.getConfig().id)
        {
            int64_t packetTime = packet.preambleTime;
            if(packetTime<10)
            {
                packetTime = packet.packetTime-getTimeOnAir(packet.data.size())/1000-5;
                if(packetTime<0)
                    packetTime = 0;
            }
            else
                packetTime -= 10;
            cPeer *peer = NULL;
            auto itr = m_peers.find(ph->src);
            if(itr!=m_peers.end())
            {
                peer = & itr->second;
            }
            else
            {
                atNetworkConfig &netConfig = m_global.getNetworkConfig();
                cPeer newPeer;
                m_peers[ph->src] = newPeer;
                peer = & m_peers[ph->src];
                peer->init(netConfig.password.c_str(),netConfig.password.length(),ph->src,netConfig.networkId);
            }
            peer->RSSI = RSSI;
            peer->SNR = SNR;
            int16_t size = packet.data.size()-sizeof(sNetworkPacketHeader);
            data += sizeof(sNetworkPacketHeader);            
            if(size && peer->geCryptoEngine().decode(data,size))
            {
                uint8_t crc = esp_crc8_le(0,data,size);
                //for(int i=0; i<size; i++)
                //    DEBUG_MSG("%02X ",data[i]);
                //DEBUG_MSG("\n");
                if(crc==ph->crc)
                {
                    int64_t now = millis();
                    //DEBUG_MSG("TS %02X,  %d %d\n",ph->src, ph->timeslot,ph->timeslotNext);
                    peer->updateTimeslot(packetTime,m_timeslotDuration,ph->timeslot,ph->timeslotNext,now);
                    if(ph->src<m_global.getConfig().id)
                    {
                        for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
                        {
                            if(m_timeslots[i].inUse && (m_timeslots[i].timeslot==ph->timeslot || m_timeslots[i].timeslot==ph->timeslotNext))
                            {
                                rebuildTimeslotsMap();
                                m_timeslots[i].timeslot = findTimeslotAround(m_timeslots[i].timeslot);
                                if(m_timeslots[i].timeslot==TIMESLOTS_MAX)
                                    m_timeslots[i].inUse = 0;
                            }
                        }
                    }

                    while(size>=sizeof(sNetworkPackeItemHeader))
                    {
                        sNetworkPackeItemHeader *pih = (sNetworkPackeItemHeader *)data;
                        if(pih->size==0)
                        {
                            DEBUG_MSG("Zero packet item size\n");
                            break;
                        }
                        if(pih->size<=size)
                        {
                            processPacketItem(peer, pih,&data[sizeof(sNetworkPackeItemHeader)],pih->size-sizeof(sNetworkPackeItemHeader));
                        }
                        else
                        {
                            DEBUG_MSG("Bad packet item size %d %d\n",size,pih->size);
                            break;
                        }
                        data += pih->size;
                        size -= pih->size;
                    }
                }
                else
                {
                    DEBUG_MSG("CRC does not match %02X %02X\n",(int)crc,(int)ph->crc);
                }
            }
            peer->updateLastPacketTime();
        }
    }
    m_lastPeerReport = NULL;
}

void cNetworkProcessor::updateConfig()
{
    atNetworkConfig &netConfig = m_global.getNetworkConfig();
    m_cryptoEngine.initWithKey(netConfig.password.c_str(),netConfig.password.length(), m_global.getConfig().id,netConfig.networkId);
    m_reportInterval = m_global.getConfig().reportInterval;
    for(auto itr=m_peers.begin(); itr!=m_peers.end(); itr++)
    {
        itr->second.init(netConfig.password.c_str(),netConfig.password.length(),itr->first,netConfig.networkId);
    }
    m_maxPacketDuration = getTimeOnAir(TIMESLOT_MAX_SYMBOLS_LEN)/1000;
    m_timeslotDuration = ((int32_t)(getTimeOnAir(TIMESLOT_MAX_SYMBOLS_LEN)*1.2)/10000)*10+30;
    if(m_timeslotDuration<TIMESLOT_MIN_DURATION)
        m_timeslotDuration = TIMESLOT_MIN_DURATION;
    DEBUG_MSG("Timeslot duration %d\n",(int)m_timeslotDuration);
}

uint8_t cNetworkProcessor::getActivePeersCount()
{
    return m_peers.size();
}

uint8_t cNetworkProcessor::calcMaxTimeslotsToUse()
{
    uint8_t result = 1;
    uint8_t peersCnt = getActivePeersCount();
    if(peersCnt==0)
        result = TIMESLOTS_PERIOD / std::max(m_reportInterval,m_timeslotDuration);
    else
        result = TIMESLOTS_PERIOD / std::max((int)m_reportInterval,m_timeslotDuration*(peersCnt+1));

    if(result<1)
        result = 1;
    else
    if(result>TIMESLOTS_PER_PEER_MAX)
        result = TIMESLOTS_PER_PEER_MAX;
    return result;
}

uint8_t cNetworkProcessor::getTimeslotsCount()
{
    uint8_t result = 0;
    for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
    {
        if(m_timeslots[i].inUse)
        {
            result++;
        }
    }
    return result;
}

bool cNetworkProcessor::isTimeslotExist(uint8_t timeslot)
{
    for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
    {
        if(m_timeslots[i].inUse)
        {
            if(m_timeslots[i].timeslot==timeslot)
                return true;
        }
    }
    return false;
}

uint8_t cNetworkProcessor::findTimeslotAround(uint8_t targetTimeslot)
{
    if(m_timeslotsMap[targetTimeslot]==NETWORK_NODE_ID_MAX)
        return targetTimeslot;

    uint8_t distance = 2;
    uint8_t maxTimeslots = TIMESLOTS_PERIOD/m_timeslotDuration;
    while(distance<TIMESLOTS_MAX/2)
    {
        bool s = (esp_random()%256)>=128;

        int16_t ts;
        if(s)
            ts = (targetTimeslot+distance)%maxTimeslots;
        else
            ts = (targetTimeslot-distance+maxTimeslots)%maxTimeslots;
        if(m_timeslotsMap[ts]==NETWORK_NODE_ID_MAX && !isTimeslotExist(ts))
            return ts;

        if(!s)
            ts = (targetTimeslot+distance)%maxTimeslots;
        else
            ts = (targetTimeslot-distance+maxTimeslots)%maxTimeslots;
        if(m_timeslotsMap[ts]==NETWORK_NODE_ID_MAX && !isTimeslotExist(ts))
            return ts;
        if(distance & 0x1)
            distance += 2;
        else
            distance--;
    }
    return TIMESLOTS_MAX;
}

void cNetworkProcessor::rebuildTimeslotsMap()
{
    memset(m_timeslotsMap,NETWORK_NODE_ID_MAX,sizeof(m_timeslotsMap));
    for(auto itr=m_peers.begin(); itr!=m_peers.end(); itr++)
    {
        sPeerTimeslot *peerTimeslots = itr->second.getTimeslots();
        for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
        {
            if(peerTimeslots[i].inUse && peerTimeslots[i].timeslot<TIMESLOTS_MAX)
                if(m_timeslotsMap[peerTimeslots[i].timeslot]>itr->first)
                    m_timeslotsMap[peerTimeslots[i].timeslot]=itr->first;
        }
    }
}

void cNetworkProcessor::rebuildTimeslots()
{
    uint8_t needTimeslots = calcMaxTimeslotsToUse();
    if(needTimeslots!=m_lastNeedTimeslots && needTimeslots>0)
    {
        m_lastNeedTimeslots = needTimeslots;
        uint8_t maxTimeslots = TIMESLOTS_PERIOD/m_timeslotDuration;
        rebuildTimeslotsMap();

        int32_t interval = TIMESLOTS_PERIOD/needTimeslots;
        double intervalInTimeslots = TIMESLOTS_PERIOD/(double)needTimeslots/m_timeslotDuration;
        //DEBUG_MSG("rebuildTS %d %d %f\n",(int)needTimeslots,interval,intervalInTimeslots);
        memset(m_timeslots,0,sizeof(m_timeslots));
        uint8_t ts = findTimeslotAround((esp_random()+(int)(m_global.getConfig().id*intervalInTimeslots/4)) %(int)intervalInTimeslots);
        //DEBUG_MSG("FTS %d\n",(int)ts);
        uint8_t firstTs = ts;
        int tsPos = 0;
        while(ts!=TIMESLOTS_MAX && needTimeslots>0 && (ts>=firstTs || (ts+(int)intervalInTimeslots)<=firstTs))
        {
            m_timeslots[tsPos].inUse = 1;
            m_timeslots[tsPos].timeslot = ts;
            m_timeslots[tsPos].lastTime = m_lastSendTime;
            tsPos++;
            needTimeslots--;
            if(needTimeslots>0)
            {
                ts = findTimeslotAround(((int)(firstTs+tsPos*intervalInTimeslots))%maxTimeslots);
                //DEBUG_MSG("TS %d\n",(int)ts);
            }
        }
    }
}

void cNetworkProcessor::sendPacket(uint8_t timeslot)
{
    sNetworkPacket packet;
    sNetworkPacketHeader *ph;
    packet.data.resize(m_bufferSize+sizeof(sNetworkPacketHeader));
    uint8_t *p =  packet.data.data();
    ph = (sNetworkPacketHeader *)p;
    ph->src = m_global.getConfig().id;
    ph->netId = m_global.getNetworkConfig().networkId;
    ph->timeslot = timeslot;
    ph->timeslotNext = getTimeslotNext(timeslot);
    ph->crc = m_bufferCRC;
    memcpy(&p[sizeof(sNetworkPacketHeader)],m_buffer,m_bufferSize);
    m_lastSendTime = millis();
    //DEBUG_MSG("%d Send ts %d %d\n",millis(), (int)ph->timeslot, (int)ph->timeslotNext);
    if(m_interface)
        m_interface->sendPacket(packet);
}

bool cNetworkProcessor::needRequestInfo(int64_t now)
{
    for(auto itr=m_peers.begin(); itr!=m_peers.end(); itr++)
    {
        if(itr->second.needRequestInfo(now,m_global.getNetworkConfig().requestInfoInterval))
            return true;
    }

    return false;
}

void cNetworkProcessor::buildRequestInfoPacket(int64_t now)
{
    sNetworkPackeItemHeader itemHeader;
    memset(&itemHeader,0,sizeof(itemHeader));
    itemHeader.type = PT_INFO_REQUEST;
    startPacketItem(itemHeader);

    int32_t requestPosition = m_bufferSize;
    sNpInfoRequest data;
    memset(&data,0,sizeof(data));
    putPacketData(&data,sizeof(data));
    int cnt=0;
    for(auto itr=m_peers.begin(); itr!=m_peers.end(); itr++)
    {
        if(itr->second.needRequestInfo(now,m_global.getNetworkConfig().requestInfoInterval))
        {
            sNpInfoRequestItem ri;
            ri.id = itr->first;
            if(putPacketData(&ri,sizeof(ri)))
            {
                cnt++;
                itr->second.updateLastRequestInfo(now);
            }
            else
                break;
        }
    }

    sNpInfoRequest *pData =(sNpInfoRequest *)&m_buffer[requestPosition];
    pData->count = cnt;

    endPacketItem();    
}

void cNetworkProcessor::sendReport(uint8_t timeslot)
{
    startPacket();
    sNpReportItem report = buildReportPacket();
    if(m_infoRequested)
    {
        m_infoRequested = false;
        buildInfoPacket();
    }
    else
    {
        int64_t now = millis();
        if(needRequestInfo(now))
        {
            buildRequestInfoPacket(now);
        }
        else
        {
            m_retransmitCounter++;
            if(m_retransmitCounter&0x1)
            {
                cPeer *retransmitPeer = NULL;
                int64_t retransmitTime = now;
                for(auto itr=m_peers.begin(); itr!=m_peers.end(); itr++)
                {
                    if(itr->second.retransmitRating>0 && (now-itr->second.getLastPacketTime())<10000 && (itr->second.getLastPacketTime()>itr->second.getLastRetransmit()) && (now-itr->second.getLastRetransmit())>=1000 && itr->second.isValidLocation())
                    //if(itr->second.retransmitRating>0 && (now-itr->second.getLastPacketTime())<10000 && (now-itr->second.getLastRetransmit())>=1000 && itr->second.isValidLocation())
                    {
                        if(retransmitTime>itr->second.getLastRetransmit())
                        {
                            retransmitTime = itr->second.getLastRetransmit();
                            retransmitPeer = &itr->second;
                        }
                    }
                }
                if(retransmitPeer && (now-retransmitTime)>=1000)
                {
                    //DEBUG_MSG("Send remote %02X\n",retransmitPeer->getId());
                    retransmitPeer->updateLastRetransmit();
                    buildRemoteReportPacket(report,*retransmitPeer);
                }
            }
        }
    }
    endPacket();
    sendPacket(timeslot);
}
//int yyy=0;
uint8_t cNetworkProcessor::getCurrentTimeslot(int64_t now)
{
    int64_t offset = 0;
    uint8_t offsetId = m_global.getConfig().id;
    for(auto itr=m_peers.begin(); itr!=m_peers.end(); itr++)
    {
        if(itr->first<offsetId && itr->second.getPeerTimeOffset()!=INVALID_TIME_OFFSET)
        {
            offsetId = itr->first;
            offset = itr->second.getPeerTimeOffset();
        }
    }
    now += offset;
    //if(yyy++%20==0)
    //    DEBUG_MSG("Offset %d %d\n",(int)offset,(int)(now%TIMESLOTS_PERIOD)/m_timeslotDuration);
    return (now%TIMESLOTS_PERIOD)/m_timeslotDuration;
}

uint8_t cNetworkProcessor::getTimeslotNext(uint8_t timeslot)
{
    uint8_t result = TIMESLOTS_MAX;
    for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
    {
        if(m_timeslots[i].inUse && m_timeslots[i].timeslot>timeslot && m_timeslots[i].timeslot<result)
        {
            result = m_timeslots[i].timeslot;
        }
    }

    if(result==TIMESLOTS_MAX)
    {
        result = timeslot;
        for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
        {
            if(m_timeslots[i].inUse && m_timeslots[i].timeslot<timeslot && m_timeslots[i].timeslot<result)
            {
                result = m_timeslots[i].timeslot;
            }
        }
    }

    return result;
}

bool cNetworkProcessor::checkTimeslot(int64_t now,uint8_t timeslot)
{
    for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
    {
        if(m_timeslots[i].inUse && m_timeslots[i].timeslot==timeslot)
        {
            if((now-m_timeslots[i].lastTime)>m_timeslotDuration*2)
            {
                m_timeslots[i].lastTime = now;
                return true;
            }
            break;
        }
    }
    return false;
}

void cNetworkProcessor::loop()
{
    int64_t now = millis();
    if(m_timeBase<=0)
        m_timeBase = now;
    if((now-m_timeBase)>=TIMESLOT_LEARNING_TIME)
    {
        if((now-m_lastRebuildTimeslots)>=TIMESLOTS_PERIOD)
        {
            m_lastRebuildTimeslots = now;
            rebuildTimeslots();
        }
        if((now-m_lastInfoTime)>=60000)
        {
            m_infoRequested = true;
        }
        uint8_t timeslot = getCurrentTimeslot(now);
        if(checkTimeslot(now,timeslot))
        {
            if(m_infoRequested)
                m_lastInfoTime = now;
            m_lastReportTime = now;
            sendReport(timeslot);
        }
    }
}

void cNetworkProcessor::startPacket()
{
    m_packetItemHeaderPosition = 0;
    m_bufferSize = 0;
}

void cNetworkProcessor::endPacket()
{
    m_bufferCRC = esp_crc8_le(0, m_buffer,m_bufferSize);
    m_cryptoEngine.encode(m_buffer,m_bufferSize);
}

void cNetworkProcessor::startPacketItem(sNetworkPackeItemHeader &itemHeader)
{
    m_packetItemHeaderPosition = m_bufferSize;
    putPacketData(&itemHeader,sizeof(itemHeader));
}

void cNetworkProcessor::endPacketItem()
{
    sNetworkPackeItemHeader *itemHeader = (sNetworkPackeItemHeader *) &m_buffer[m_packetItemHeaderPosition];
    itemHeader->size = m_bufferSize-m_packetItemHeaderPosition;
}

bool cNetworkProcessor::putPacketData(void *data,uint16_t size)
{
    auto pos = m_bufferSize;
    if((pos+size)<MAX_NETWORK_PACKET_SIZE)
    {
        m_bufferSize+=size;
        memcpy(&m_buffer[pos],data,size);
        return true;
    }
    return false;
}

void cNetworkProcessor::buildInfoPacket()
{
    sNetworkPackeItemHeader itemHeader;
    memset(&itemHeader,0,sizeof(itemHeader));
    itemHeader.type = PT_INFO_RESPONSE;
    startPacketItem(itemHeader);

    sNpInfoResponse data;
    memset(&data,0,sizeof(data));
    
    strncpy(data.name,m_global.getConfig().name.c_str(),MAX_NETWORK_USER_NAME-1);
    data.symbol = m_global.getConfig().symbol;

    putPacketData(&data,sizeof(data));

    endPacketItem();    
}

static inline int32_t getBitsSize(uint8_t bits)
{
    return ((int32_t)1 << bits);
}

void cNetworkProcessor::buildRemoteReportPacket(const sNpReportItem &report, cPeer &peer)
{
    sNetworkPackeItemHeader itemHeader;
    
    memset(&itemHeader,0,sizeof(itemHeader));
    int32_t peerLat = peer.lat*1e-6;
    int32_t peerLng = peer.lng*1e-6;
    int32_t peerAlt = peer.alt;

    sNpReportRemoteHeader header;
    memset(&header,0,sizeof(header));
    header.src = peer.getId();
    sPeerTimeslot *ts = peer.getTimeslots();
    std::list<int> tsList;
    for(int i=0; i<TIMESLOTS_PER_PEER_MAX; i++)
    {
        if(ts[i].inUse)
        {
            tsList.push_back(ts[i].timeslot);
        }
    }
    tsList.sort();
    if(tsList.size()>1)
    {
        int maxTSDiff = 0;
        int lastTS = -1;
        for(auto itr=tsList.begin(); itr!=tsList.end(); itr++)
        {
            if(lastTS==-1)
            {
                lastTS = *itr;
            }
            else
            {
                int diff = *itr - lastTS;
                lastTS = *itr;
                if(diff>maxTSDiff)
                    maxTSDiff = diff;
            }
        }
        if(maxTSDiff>=getBitsSize(3))
        {
            header.ts.d4.flag = 0;    
            lastTS = -1;
            int cnt = 0;
            for(auto itr=tsList.begin(); itr!=tsList.end(); itr++)
            {
                if(lastTS==-1)
                {
                    lastTS = *itr;
                    header.ts.d4.t0 = lastTS;
                }
                else
                {
                    int diff = *itr - lastTS;
                    lastTS = *itr;
                    cnt++;
                    if(diff<getBitsSize(4))
                        header.ts.d4.set(cnt,diff);
                    else
                        break;
                }
                if(cnt>=header.ts.d4.size())
                    break;
            }
        }
        else
        {
            header.ts.d3.flag = 1;
            lastTS = -1;
            int cnt = 0;
            for(auto itr=tsList.begin(); itr!=tsList.end(); itr++)
            {
                if(lastTS==-1)
                {
                    lastTS = *itr;
                    header.ts.d3.t0 = lastTS;
                }
                else
                {
                    int diff = *itr - lastTS;
                    lastTS = *itr;
                    cnt++;
                    if(diff<getBitsSize(3))
                        header.ts.d3.set(cnt,diff);
                    else
                        break;
                }
                if(cnt>=header.ts.d3.size())
                    break;
            }
        }
    }
    else
    {
        header.ts.d4.flag = 0;
        if(tsList.size()==1)
            header.ts.d4.t0 = tsList.front();
        else
            header.ts.d4.t0 = TIMESLOTS_MAX-1;
    }

    //D1 report
    if(abs(peerLat - report.lat)<getBitsSize(RID1SL-1) &&
       abs(peerLng - report.lng)<getBitsSize(RID1SL-1) &&
       abs(peerAlt - report.alt)<getBitsSize(RID1SA-1))
    {
        itemHeader.type = PT_REPORT_REMOTE_D1;
        startPacketItem(itemHeader);

        sNpReportItemD1 report;
        memset(&report,0,sizeof(report));
        
        report.lat = peerLat - report.lat;
        report.lng = peerLng - report.lng;
        report.alt = peerAlt - report.alt;
        report.heading = peer.heading;
        report.status = peer.status;
        report.options = peer.options;
        report.hacc = peer.hacc;

        putPacketData(&header,sizeof(header));
        putPacketData(&report,sizeof(report));
    }
    else
    {
        itemHeader.type = PT_REPORT_REMOTE;
        startPacketItem(itemHeader);

        sNpReportItem report;
        memset(&report,0,sizeof(report));
        
        report.lat = peerLat;
        report.lng = peerLng;
        report.alt = peerAlt;
        report.heading = peer.heading;
        report.status = peer.status;
        report.options = peer.options;
        report.hacc = peer.hacc;

        putPacketData(&header,sizeof(header));
        putPacketData(&report,sizeof(report));        
    }

    endPacketItem();
}

sNpReportItem cNetworkProcessor::buildReportPacket()
{
    sNetworkPackeItemHeader itemHeader;
    memset(&itemHeader,0,sizeof(itemHeader));
    itemHeader.type = PT_REPORT;
    startPacketItem(itemHeader);

    sNpReportItem report;
    memset(&report,0,sizeof(report));
    
    report.lat = m_global.m_gpsStatus->getLatitude()/10;
    report.lng = m_global.m_gpsStatus->getLongitude()/10;
    report.alt = m_global.m_gpsStatus->getAltitude();
    report.heading = m_global.m_imuStatus->getHeading()/100;
    report.status = m_global.m_playerStatus.status;
    report.options = m_global.m_playerStatus.options;
    report.hacc = std::min(m_global.m_gpsStatus->getHAcc()/100,255U);

    putPacketData(&report,sizeof(report));

    endPacketItem();
    return report;
}

void cPeer::updateReport(const sNpReportItem * report)
{
    memcpy(&m_report,report,sizeof(sNpReportItem));
    lat = m_report.lat*1e-6;
    lng = m_report.lng*1e-6;
    alt = m_report.alt;
    heading = m_report.heading;
    status = m_report.status;
    options = m_report.options;
    positionChanged = true;
    hacc = m_report.hacc;
    updateXY();
}


uint32_t cNetworkProcessor::getTimeOnAir(size_t len) {
    cLoraChannelCondfig &channelConfig = m_global.getNetworkConfig().channelConfig;
  // everything is in microseconds to allow integer arithmetic
  // some constants have .25, these are multiplied by 4, and have _x4 postfix to indicate that fact
    uint32_t symbolLength_us = ((uint32_t)(1000 * 10) << channelConfig.sf) / (channelConfig.bw * 10) ;
    uint8_t sfCoeff1_x4 = 17; // (4.25 * 4)
    uint8_t sfCoeff2 = 8;
    if(channelConfig.sf == 5 || channelConfig.sf == 6) {
      sfCoeff1_x4 = 25; // 6.25 * 4
      sfCoeff2 = 0;
    }
    uint8_t sfDivisor = 4*channelConfig.sf;
    if(symbolLength_us >= 16000) {
      sfDivisor = 4*(channelConfig.sf - 2);
    }
    const int8_t bitsPerCrc = 16;
    const int8_t N_symbol_header = 20;

    // numerator of equation in section 6.1.4 of SX1268 datasheet v1.1 (might not actually be bitcount, but it has len * 8)
    int16_t bitCount = (int16_t) 8 * len + 1 * bitsPerCrc - 4 * channelConfig.sf  + sfCoeff2 + N_symbol_header;
    if(bitCount < 0) {
      bitCount = 0;
    }
    // add (sfDivisor) - 1 to the numerator to give integer CEIL(...)
    uint16_t nPreCodedSymbols = (bitCount + (sfDivisor - 1)) / (sfDivisor);

    // preamble can be 65k, therefore nSymbol_x4 needs to be 32 bit
    uint32_t nSymbol_x4 = (channelConfig.preambleLength + 8) * 4 + sfCoeff1_x4 + nPreCodedSymbols * channelConfig.cr * 4;

    return((symbolLength_us * nSymbol_x4) / 4);
}

