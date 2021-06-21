//==========================================================================
//  LOGBUFFER.CC - part of
//
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2017 Andras Varga
  Copyright (C) 2006-2017 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#include <cstring>
#include "omnetpp/cmodule.h"
#include "omnetpp/cmessage.h"
#include "common/stringutil.h"
#include "logbuffer.h"
#include "qtutil.h"

#define emit    // Qt...

using namespace omnetpp::common;

namespace omnetpp {
namespace qtenv {

LogBuffer::Entry::Entry(eventnumber_t e, simtime_t t, cModule *mod, const char *banner)
    : eventNumber(e), simtime(t), componentId(mod ? (mod->getId()) : 0), banner(banner)
{

}

LogBuffer::Entry::~Entry()
{
    delete[] banner;
    for (auto & line : lines) {
        delete[] line.prefix;
        delete[] line.line;
    }
    for (auto & msg : msgs) {
        delete msg.msg;
    }
}

//----

void LogBuffer::addEvent(eventnumber_t e, simtime_t t, cModule *mod, const char *banner)
{
    Entry *entry = new Entry(e, t, mod, opp_strdup(banner));
    entries.push_back(entry);
    discardEventsIfLimitExceeded();

    emit logEntryAdded();
}

void LogBuffer::addInitialize(cComponent *component, const char *banner)
{
    if (entries.empty()) {
        Entry *entry = new Entry(0, simTime(), getSimulation()->getSystemModule(), opp_strdup("** Initializing network\n"));
        entries.push_back(entry);
    }

    Entry *entry = entries.back();
    cComponent *contextComponent = getSimulation()->getContext();
    int contextComponentId = contextComponent ? contextComponent->getId() : 0;
    entry->lines.push_back(Line(contextComponentId, LogLevel::LOGLEVEL_INFO, nullptr, opp_strdup(banner)));

    emit logLineAdded();
}



void LogBuffer::addLogLine(LogLevel logLevel, const char *prefix, const char *text, int len)
{
    if (entries.empty()) {
        Entry *entry = new Entry(0, simTime(), nullptr, nullptr);
        entries.push_back(entry);
    }

    // FIXME if last line is "info" then we cannot append to it! create new entry with empty banner?

    Entry *entry = entries.back();
    cComponent *contextComponent = getSimulation()->getContext();
    int contextComponentId = contextComponent ? contextComponent->getId() : 0;
    entry->lines.push_back(Line(contextComponentId, logLevel, opp_strdup(prefix), opp_strndup(text, len)));

    emit logLineAdded();
}

void LogBuffer::addInfo(const char *text, int len)
{
    // TODO ha inline info (contextmodule!=nullptr), sima logline-kent adjuk hozza!!!!
    Entry *entry = new Entry(0, simTime(), nullptr, opp_strndup(text, len));
    entries.push_back(entry);
    discardEventsIfLimitExceeded();

    emit logEntryAdded();
}

void LogBuffer::beginSend(cMessage *msg, const SendOptions& options)
{
    if (entries.empty()) {
        // this is likely the initialize() phase -- hence no banner
        addEvent(0, SIMTIME_ZERO, nullptr, nullptr);
        Entry *entry = entries.back();
        entry->componentId = -1;
    }

    // FIXME if last line is "info" then we cannot append to it! create new entry with empty banner?

    Entry *entry = entries.back();
    entry->msgs.push_back(MessageSend());
    MessageSend& msgsend = entry->msgs.back();
    msgsend.msg = nullptr;
    msgsend.hopModuleIds.push_back(msg->getSenderModuleId());
}

void LogBuffer::messageSendDirect(cMessage *msg, cGate *toGate, const ChannelResult& result)
{
    ASSERT(!entries.empty());
    Entry *entry = entries.back();
    MessageSend& msgsend = entry->msgs.back();
    ASSERT(!msgsend.msg || msgsend.msg->getId() == msg->getId());

    std::vector<cModule *> hops;
    resolveSendDirectHops(msg->getSenderModule(), toGate->getOwnerModule(), hops);

    for (unsigned i = 1; i < hops.size(); i++)  // iterate from 1 because src module is already in hopModuleIds
        msgsend.hopModuleIds.push_back(hops[i]->getId());
}

void LogBuffer::messageSendHop(cMessage *msg, cGate *srcGate)
{
    ASSERT(!entries.empty());
    Entry *entry = entries.back();
    MessageSend& msgsend = entry->msgs.back();
    ASSERT(!msgsend.msg || msgsend.msg->getId() == msg->getId());
    msgsend.hopModuleIds.push_back(srcGate->getNextGate()->getOwnerModule()->getId());
}

void LogBuffer::messageSendHop(cMessage *msg, cGate *srcGate, const cChannel::Result& result)
{
    ASSERT(!entries.empty());
    Entry *entry = entries.back();
    MessageSend& msgsend = entry->msgs.back();
    ASSERT(!msgsend.msg || msgsend.msg->getId() == msg->getId());

    if (result.discard) {
        // the message was discarded, so it will not arrive, endSend() will not be called,
        // but we have to make a copy anyway
        msgsend.msg = msg->privateDup();

        // storing the copy for animation
        messageDups.insert({msg, msgsend.msg});

        // clearing the previous arrival module/gate, since it did not really arrive
        msgsend.msg->setArrival(0, 0);
        msgsend.discarded = true;
    }

    msgsend.hopModuleIds.push_back(srcGate->getNextGate()->getOwnerModule()->getId());
}

void LogBuffer::endSend(cMessage *msg)
{
    ASSERT(!entries.empty());
    Entry *entry = entries.back();
    MessageSend& msgsend = entry->msgs.back();
    // the message has arrived, we have to make a copy
    msgsend.msg = msg->privateDup();

    // storing the copy for animation
    messageDups.insert({msg, msgsend.msg});

    emit messageSendAdded();
}

void LogBuffer::delivery(cMessage *msg)
{
    // most likely we just got out of express mode
    if (!getLastMessageDup(msg))
        // storing a copy for animation
        messageDups.insert({msg, msg->privateDup()});
}

void LogBuffer::setMaxNumEntries(int limit)
{
    maxNumEntries = limit;
    discardEventsIfLimitExceeded();
}

void LogBuffer::discardEventsIfLimitExceeded()
{
    // discard first entry
    while (maxNumEntries > 0 && entries.size() > maxNumEntries) {
        auto discardedEntry = entries.front();
        entries.pop_front();
        entriesDiscarded++;
        emit entryDiscarded(discardedEntry);
        delete discardedEntry;
    }
}

void LogBuffer::clear()
{
    for (int i = 0; i < entries.size(); i++)
        delete entries[i];
    entries.clear();
    entriesDiscarded = 0;
    messageDups.clear();
}

int LogBuffer::findEntryByEventNumber(eventnumber_t eventNumber)
{
    int first = 0, last = entries.size()-1;
    int middle = (first + last)/2;
    while (first <= last) {
        if (entries[middle]->eventNumber < eventNumber)
            first = middle + 1;
        else if (entries[middle]->eventNumber == eventNumber)
            return middle;
        else
            last = middle - 1;

        middle = (first + last)/2;
    }
    return -1;
}

LogBuffer::Entry *LogBuffer::getEntryByEventNumber(eventnumber_t eventNumber)
{
    int i = findEntryByEventNumber(eventNumber);
    return i == -1 ? nullptr : entries[i];
}

cMessage *LogBuffer::getLastMessageDup(cMessage *of)
{
    auto range = messageDups.equal_range(of);
    if (range.first == range.second)
        return nullptr;
    return (--range.second)->second;
}

void LogBuffer::dump() const
{
    printf("LogBuffer: %d entries\n", (int)entries.size());

    for (int i = 0; i < entries.size(); i++) {
        const Entry *entry = entries[i];
        printf("[%d] #%" PRId64 " t=%s componentId=%d: %s", i, entry->eventNumber, SIMTIME_STR(entry->simtime), entry->componentId, entry->banner);
        for (const auto & line : entry->lines)
            printf("\t[l%d]:%s%s", i, line.prefix, line.line);
        for (const auto & msg : entry->msgs) {
            printf("\t[l%d]:%s\n", i, msg.msg->str().c_str());
            for (auto id : msg.hopModuleIds)
                printf("\t\t[l%d]:%s\n", i, getSimulation()->getComponent(id)->getFullPath().c_str());
        }
    }
}

}  // namespace qtenv
}  // namespace omnetpp

