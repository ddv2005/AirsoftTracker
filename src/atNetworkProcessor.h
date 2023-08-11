#pragma once
#include <Arduino.h>
#include "config.h"
#include "atNetworkStructs.h"
#include "atglobal.h"
#include <vector>
#include <map>

#pragma pack(push, 1)
struct sPeerTimeslot
{
    uint8_t timeslot:7;
    uint8_t inUse:1;
    int64_t lastTime;
};
#pragma pack(pop)

struct sNetworkPacket
{
    int64_t packetTime;
    int64_t preambleTime;
    std::vector<byte> data;
};

class cNetworkProcessorInterface
{
public:
    virtual void sendPacket(const sNetworkPacket &packet)=0;
};
#define INVALID_TIME_OFFSET 0xFFFFFFFF
#define CK_SIZE 32
#define MAX_NETWORK_PACKET_SIZE 64
class cCryptoEngine
{
protected:
    uint8_t m_key[CK_SIZE];
public:
    cCryptoEngine();
    void initWithKey(const char *key, uint8_t  size, uint8_t id,  uint8_t netId);
    void encode(uint8_t *data, uint16_t size);
    bool decode(uint8_t *data, uint16_t size);
};

class cPeer:public sPOI
{
protected:
    uint8_t m_id;
    cCryptoEngine m_cryptoEngine;
    int64_t m_lastPacketTime;
    int64_t m_lastRequestInfo;
    int64_t m_lastRetransmit;
    sNpReportItem m_report;
    sPeerTimeslot m_timeslots[TIMESLOTS_PER_PEER_MAX];
    int64_t m_peerTimeOffset=INVALID_TIME_OFFSET;
public:
    cPeer();
    void init(const char *key, uint8_t  size, uint8_t id,  uint8_t netId);
    const char * getName()
    {
        return name.c_str();
    }

    char getSymbol()
    {
        return symbol;
    }

    void updateInfo(const sNpInfoResponse *info)
    {
        name = info->name;
        symbol = info->symbol;
    }

    void updateLastPacketTime()
    {
        lastUpdate = m_lastPacketTime = millis();
    }

    void updateLastRetransmit()
    {
        m_lastRetransmit = millis();
    }

    int64_t getLastRetransmit()
    {
        return m_lastRetransmit;
    }

    int64_t getLastPacketTime()
    {
        return m_lastPacketTime;
    }

    uint8_t getId() const
    {
        return m_id;
    }

    sNpReportItem & getReport()
    {
        return m_report;
    }

    void updateReport(const sNpReportItem * report);

    cCryptoEngine & geCryptoEngine()
    {
        return m_cryptoEngine;
    }

    bool needRequestInfo(int64_t now, int64_t interval);

    void updateLastRequestInfo(int64_t now)
    {
        m_lastRequestInfo = now;
    }

    void updateTimeslot(uint8_t timeslot, int64_t now);
    void updateTimeslot(int64_t packetTime, uint16_t timeslotDuration, uint8_t timeslot,uint8_t timeslotNext, int64_t now);
    uint16_t getTimeslotOffset(uint8_t timeslot, uint16_t timeslotDuration);
    sPeerTimeslot *getTimeslots() { return m_timeslots; }
    int64_t getPeerTimeOffset() { return m_peerTimeOffset; }
};

typedef std::map<uint8_t,cPeer> cPeers;

class cNetworkProcessor
{
protected:
    atGlobal &m_global;
    cNetworkProcessorInterface *m_interface;
    cCryptoEngine m_cryptoEngine;
    int64_t m_lastReportTime;
    int64_t m_lastInfoTime;
    uint16_t m_reportInterval;
    uint8_t m_buffer[MAX_NETWORK_PACKET_SIZE];
    uint16_t m_bufferSize;
    uint16_t m_packetItemHeaderPosition;
    bool m_infoRequested;
    cPeers m_peers;
    uint8_t m_bufferCRC;
    uint16_t m_timeslotDuration=0;
    uint16_t m_maxPacketDuration=0;
    uint8_t m_timeslotsMap[TIMESLOTS_MAX];
    sPeerTimeslot m_timeslots[TIMESLOTS_PER_PEER_MAX];
    int64_t m_timeBase=0;
    int64_t m_lastRebuildTimeslots = 0;
    int64_t m_lastSendTime = 0;
    uint8_t m_lastNeedTimeslots=0;
    uint32_t m_retransmitCounter = 0;
    sNpReportItem *m_lastPeerReport = NULL;

    void startPacket();
    void endPacket();
    void startPacketItem(sNetworkPackeItemHeader &itemHeader);
    void endPacketItem();
    bool putPacketData(void *data,uint16_t size);

    bool needRequestInfo(int64_t now);
    sNpReportItem buildReportPacket();
    void buildRemoteReportPacket(const sNpReportItem &report, cPeer &peer);
    void buildInfoPacket();
    void buildRequestInfoPacket(int64_t now);
    void sendPacket(uint8_t timeslot);
    void processPacketItem(cPeer *peer, const sNetworkPackeItemHeader *header, const uint8_t *data, uint16_t size);
    uint8_t getActivePeersCount();
    uint8_t calcMaxTimeslotsToUse();
    uint8_t getTimeslotsCount();
    void rebuildTimeslotsMap();
    void rebuildTimeslots();
    bool isTimeslotExist(uint8_t timeslot);
    bool checkTimeslot(int64_t now,uint8_t timeslot);
    uint8_t findTimeslotAround(uint8_t targetTimeslot);
    uint8_t getCurrentTimeslot(int64_t now);
    uint8_t getTimeslotNext(uint8_t timeslot);
public:
    cNetworkProcessor(atGlobal &global, cNetworkProcessorInterface *interface);
    void updateConfig();
    void processPacket(const sNetworkPacket &packet, float RSSI, float SNR);
    void loop();
    void sendReport(uint8_t timeslot);
    uint16_t getTimeslotDuration() { return m_timeslotDuration; }
    uint16_t getMaxPacketDuration() { return m_maxPacketDuration; }
    uint32_t getTimeOnAir(size_t len);
    cPeers &getPeers()
    {
        return m_peers;
    }
};