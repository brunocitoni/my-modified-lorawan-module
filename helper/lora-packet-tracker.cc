/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Davide Magrin <magrinda@dei.unipd.it>
 */

#include "lora-packet-tracker.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/lorawan-mac-header.h"
#include <iostream>
#include <fstream>

#include "ns3/end-device-lora-phy.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/lora-tag.h"

namespace ns3 {
namespace lorawan {
NS_LOG_COMPONENT_DEFINE ("LoraPacketTracker");



LoraPacketTracker::LoraPacketTracker ()
{
  NS_LOG_FUNCTION (this);

}

LoraPacketTracker::~LoraPacketTracker ()
{
  NS_LOG_FUNCTION (this);
}

/////////////////
// MAC metrics //
/////////////////

void
LoraPacketTracker::MacTransmissionCallback (Ptr<Packet const> packet)
{
  if (IsUplink (packet))
    {
      NS_LOG_INFO ("A new packet was sent by the MAC layer");

      MacPacketStatus status;
      status.packet = packet;
      status.sendTime = Simulator::Now ();
      status.senderId = Simulator::GetContext ();
      status.receivedTime = Time::Max ();

      m_macPacketTracker.insert (std::pair<Ptr<Packet const>, MacPacketStatus>
                                   (packet, status));

      /*std::cout << "Status: " << status.packet
    		  << ", send time: " << status.sendTime
			  << ", device ID: " << status.senderId
			  << ", time: " << Simulator::Now ()
			  << std::endl;
       */

    }
}

void
LoraPacketTracker::RequiredTransmissionsCallback (uint8_t reqTx, bool success,
                                                  Time firstAttempt,
                                                  Ptr<Packet> packet)
{
  NS_LOG_INFO ("Finished retransmission attempts for a packet");
  NS_LOG_DEBUG ("Packet: " << packet << "ReqTx " << unsigned(reqTx) <<
                ", succ: " << success << ", firstAttempt: " <<
                firstAttempt.GetSeconds ());

  RetransmissionStatus entry;
  entry.firstAttempt = firstAttempt;
  entry.finishTime = Simulator::Now ();
  entry.reTxAttempts = reqTx;
  entry.successful = success;

  m_reTransmissionTracker.insert (std::pair<Ptr<Packet>, RetransmissionStatus>
                                    (packet, entry));

}

void
LoraPacketTracker::MacGwReceptionCallback (Ptr<Packet const> packet)
{
  if (IsUplink (packet))
    {
      NS_LOG_INFO ("A packet was successfully received" <<
                   " at the MAC layer of gateway " <<
                   Simulator::GetContext ());

      //std::cout << "Packet received at MacGW: " << Simulator::GetContext () <<" at time " << Simulator::Now ().GetSeconds()<<std::endl;

      // Find the received packet in the m_macPacketTracker
      auto it = m_macPacketTracker.find (packet);
      if (it != m_macPacketTracker.end ())
        {
          (*it).second.receptionTimes.insert (std::pair<int, Time>
                                                (Simulator::GetContext (),
                                                Simulator::Now ()));
        }
      else
        {
          NS_ABORT_MSG ("Packet not found in tracker");
        }
    }
}

/////////////////
// PHY metrics //
/////////////////

void
LoraPacketTracker::TransmissionCallback (Ptr<Packet const> packet, uint32_t edId)
{
  if (IsUplink (packet))
    {
      NS_LOG_INFO ("PHY packet " << packet
                                 << " was transmitted by device "
                                 << edId);
      // Create a packetStatus
      PacketStatus status;
      status.packet = packet;
      status.sendTime = Simulator::Now ();
      status.senderId = edId;

      m_packetTracker.insert (std::pair<Ptr<Packet const>, PacketStatus> (packet, status));
    }
}

void
LoraPacketTracker::PacketReceptionCallback (Ptr<Packet const> packet, uint32_t gwId)
{

	 std::ofstream packetsOutcome; // added by BC
	 packetsOutcome.open ("packetsOutcome.txt", std::ofstream::out | std::ofstream::app);

  if (IsUplink (packet))
    {
      // Remove the successfully received packet from the list of sent ones
      NS_LOG_INFO ("PHY packet " << packet
                                 << " was successfully received at gateway "
                                 << gwId);


      if(m_packetTracker.count(packet) == 1) //first reception
        {
          std::map<Ptr<Packet const>, PacketStatus>::iterator it = m_packetTracker.find (packet);
          (*it).second.outcomes.insert (std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                           RECEIVED));
        }
      else
        {
          std::pair <std::multimap<Ptr<Packet const>, PacketStatus>::iterator, std::multimap<Ptr<Packet const>, PacketStatus>::iterator> ret;
          ret = m_packetTracker.equal_range(packet); // find all instances of received packet

          int i = 1;

          //loop through all instances and find the one which doesn't yet have an outcome at this gateway
          for(std::multimap<Ptr<Packet const>, PacketStatus>::iterator it=ret.first; it != ret.second; ++it)
            {

              if((*it).second.outcomes.find(gwId) == (*it).second.outcomes.end())
              {
                NS_LOG_DEBUG("This is copy  " << i <<" of packet "<< packet);
                NS_LOG_DEBUG("This packet was sent at " << (*it).second.sendTime);
                NS_LOG_DEBUG("It was not yet received by GW " << gwId << " logging it now as RECEIVED.");
                (*it).second.outcomes.insert(std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                           RECEIVED));
              }
              i++;
            }
        }

//      std::cout << "Packet " << packet << " successfully received at GW: " << gwId<< " at time " << Simulator::Now ().GetSeconds() <<std::endl;
      packetsOutcome << "Packet " << packet->GetUid() << " outcome: receivedSuccessfully, at GW id " << gwId<< " at time " << Simulator::Now().GetSeconds() <<";" << std::endl;
      packetsOutcome.close();
    }
}

void
LoraPacketTracker::InterferenceCallback (Ptr<Packet const> packet, uint32_t gwId)
{

	 std::ofstream packetsOutcome; // added by BC
	 packetsOutcome.open ("packetsOutcome.txt", std::ofstream::out | std::ofstream::app);
  if (IsUplink (packet))
    {
      NS_LOG_INFO ("PHY packet " << packet
                                 << " was interfered at gateway "
                                 << gwId);

      if(m_packetTracker.count(packet) == 1) //first reception
           {
             std::map<Ptr<Packet const>, PacketStatus>::iterator it = m_packetTracker.find (packet);
             (*it).second.outcomes.insert (std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                              INTERFERED));
           }
         else
           {
             std::pair <std::multimap<Ptr<Packet const>, PacketStatus>::iterator, std::multimap<Ptr<Packet const>, PacketStatus>::iterator> ret;
             ret = m_packetTracker.equal_range(packet); // find all instances of received packet

             int i = 1;

             //loop through all instances and find the one which doesn't yet have an outcome at this gateway
             for(std::multimap<Ptr<Packet const>, PacketStatus>::iterator it=ret.first; it != ret.second; ++it)
               {

                 if((*it).second.outcomes.find(gwId) == (*it).second.outcomes.end())
                 {
                   NS_LOG_INFO("This is copy  " << i <<" of packet "<< packet);
                   NS_LOG_INFO("This packet was sent at " << (*it).second.sendTime);
                   NS_LOG_INFO("It was not yet received by GW " << gwId << " logging it now as INTERFERED.");
                   (*it).second.outcomes.insert(std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                              INTERFERED));
                 }
                 i++;
               }
           }
    }

    //std::cout << "Packet " << packet << " interfered in air before arriving at GW: " << gwId<< " at time " << Simulator::Now ().GetSeconds() <<std::endl;
    packetsOutcome << "Packet " << packet->GetUid() << " outcome: interferedOnAir, before arriving at GW id " << gwId<< " at time " << Simulator::Now ().GetSeconds() << ";" << std::endl;
    packetsOutcome.close();
}

void
LoraPacketTracker::NoMoreReceiversCallback (Ptr<Packet const> packet, uint32_t gwId)
{
	 std::ofstream packetsOutcome; // added by BC
	 packetsOutcome.open ("packetsOutcome.txt", std::ofstream::out | std::ofstream::app);
  if (IsUplink (packet))
    {
      NS_LOG_INFO ("PHY packet " << packet
                                 << " was lost because no more receivers at gateway "
                                 << gwId);
      if(m_packetTracker.count(packet) == 1) //first reception
              {
                std::map<Ptr<Packet const>, PacketStatus>::iterator it = m_packetTracker.find (packet);
                (*it).second.outcomes.insert (std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                                 NO_MORE_RECEIVERS));
              }
            else
              {
                std::pair <std::multimap<Ptr<Packet const>, PacketStatus>::iterator, std::multimap<Ptr<Packet const>, PacketStatus>::iterator> ret;
                ret = m_packetTracker.equal_range(packet); // find all instances of received packet

                int i = 1;

                //loop through all instances and find the one which doesn't yet have an outcome at this gateway
                for(std::multimap<Ptr<Packet const>, PacketStatus>::iterator it=ret.first; it != ret.second; ++it)
                  {

                    if((*it).second.outcomes.find(gwId) == (*it).second.outcomes.end())
                    {
                      NS_LOG_INFO("This is copy  " << i <<" of packet "<< packet);
                      NS_LOG_INFO("This packet was sent at " << (*it).second.sendTime);
                      NS_LOG_INFO("It was not yet received by GW " << gwId << " logging it now as NO_MORE_RECEIVERS.");
                      (*it).second.outcomes.insert(std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                                 NO_MORE_RECEIVERS));
                    }
                    i++;
                  }
              }
    }

  //std::cout << "Packet " << packet << " lost because no more receivers at GW: " << gwId<< " at time " << Simulator::Now ().GetSeconds() <<std::endl;
  packetsOutcome << "Packet " << packet->GetUid() << " outcome: channelSaturated, because no more receivers at GW id " << gwId<< " at time " << Simulator::Now ().GetSeconds() <<";" << std::endl;
  packetsOutcome.close();
}

void
LoraPacketTracker::UnderSensitivityCallback (Ptr<Packet const> packet, uint32_t gwId)
{

	 std::ofstream packetsOutcome; // added by BC
	 packetsOutcome.open ("packetsOutcome.txt", std::ofstream::out | std::ofstream::app);
  if (IsUplink (packet))
    {
      NS_LOG_INFO ("PHY packet " << packet
                                 << " was lost because under sensitivity at gateway "
                                 << gwId);

      if(m_packetTracker.count(packet) == 1) //first reception
              {
                std::map<Ptr<Packet const>, PacketStatus>::iterator it = m_packetTracker.find (packet);
                (*it).second.outcomes.insert (std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                                 UNDER_SENSITIVITY));
              }
            else
              {
                std::pair <std::multimap<Ptr<Packet const>, PacketStatus>::iterator, std::multimap<Ptr<Packet const>, PacketStatus>::iterator> ret;
                ret = m_packetTracker.equal_range(packet); // find all instances of received packet

                int i = 1;

                //loop through all instances and find the one which doesn't yet have an outcome at this gateway
                for(std::multimap<Ptr<Packet const>, PacketStatus>::iterator it=ret.first; it != ret.second; ++it)
                  {

                    if((*it).second.outcomes.find(gwId) == (*it).second.outcomes.end())
                    {
                      NS_LOG_INFO("This is copy  " << i <<" of packet "<< packet);
                      NS_LOG_INFO("This packet was sent at " << (*it).second.sendTime);
                      NS_LOG_INFO("It was not yet received by GW " << gwId << " logging it now as UNDER_SENSITIVITY.");
                      (*it).second.outcomes.insert(std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                                 UNDER_SENSITIVITY));
                    }
                    i++;
                  }
              }
    }

  //std::cout << "Packet " << packet << " lost because under sensitivity at GW: " << gwId<< " at time " << Simulator::Now ().GetSeconds() <<std::endl;
  packetsOutcome << "Packet " << packet->GetUid() << " outcome: underSensitivity, at GW id " << gwId<< " at time " << Simulator::Now ().GetSeconds() <<";" << std::endl;
  packetsOutcome.close();
}

void
LoraPacketTracker::LostBecauseTxCallback (Ptr<Packet const> packet, uint32_t gwId)
{
	 std::ofstream packetsOutcome; // added by BC
	 packetsOutcome.open ("packetsOutcome.txt", std::ofstream::out | std::ofstream::app);
  if (IsUplink (packet))
    {
      NS_LOG_INFO ("PHY packet " << packet
                                 << " was lost because of GW transmission at gateway "
                                 << gwId);

      if(m_packetTracker.count(packet) == 1) //first reception
              {
                std::map<Ptr<Packet const>, PacketStatus>::iterator it = m_packetTracker.find (packet);
                (*it).second.outcomes.insert (std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                                 LOST_BECAUSE_TX));
              }
            else
              {
                std::pair <std::multimap<Ptr<Packet const>, PacketStatus>::iterator, std::multimap<Ptr<Packet const>, PacketStatus>::iterator> ret;
                ret = m_packetTracker.equal_range(packet); // find all instances of received packet

                int i = 1;

                //loop through all instances and find the one which doesn't yet have an outcome at this gateway
                for(std::multimap<Ptr<Packet const>, PacketStatus>::iterator it=ret.first; it != ret.second; ++it)
                  {

                    if((*it).second.outcomes.find(gwId) == (*it).second.outcomes.end())
                    {
                      NS_LOG_INFO("This is copy  " << i <<" of packet "<< packet);
                      NS_LOG_INFO("This packet was sent at " << (*it).second.sendTime);
                      NS_LOG_INFO("It was not yet received by GW " << gwId << " logging it now as LOST_BECAUSE_TX.");
                      (*it).second.outcomes.insert(std::pair<int, enum PhyPacketOutcome> (gwId,
                                                                                 LOST_BECAUSE_TX));
                    }
                    i++;
                  }
              }
    }

  //std::cout << "Packet " << packet << " lost because GW: " << gwId<< " was also transmitting on same channel at time " << Simulator::Now ().GetSeconds() <<std::endl;
  packetsOutcome << "Packet " << packet->GetUid() << " outcome: interferedWithtxCallback, at GW id " << gwId<< " at time " << Simulator::Now ().GetSeconds() <<";" << std::endl;
  packetsOutcome.close();
}

bool
LoraPacketTracker::IsUplink (Ptr<Packet const> packet)
{
  NS_LOG_FUNCTION (this);

  LorawanMacHeader mHdr;
  Ptr<Packet> copy = packet->Copy ();
  copy->RemoveHeader (mHdr);
  return mHdr.IsUplink ();
}

////////////////////////
// Counting Functions //
////////////////////////

std::vector<int>
LoraPacketTracker::CountPhyPacketsPerGw (Time startTime, Time stopTime,
                                         int gwId)
{
  // Vector packetCounts will contain - for the interval given in the input of
  // the function, the following fields: totPacketsSent receivedPackets
  // interferedPackets noMoreGwPackets underSensitivityPackets

  std::vector<int> packetCounts (6, 0);

  for (auto itPhy = m_packetTracker.begin ();
       itPhy != m_packetTracker.end ();
       ++itPhy)
    {
      if ((*itPhy).second.sendTime >= startTime && (*itPhy).second.sendTime <= stopTime)
        {
          packetCounts.at (0)++;

          NS_LOG_DEBUG ("Dealing with packet " << (*itPhy).second.packet);
          NS_LOG_DEBUG ("This packet was received by " <<
                        (*itPhy).second.outcomes.size () << " gateways");

          if ((*itPhy).second.outcomes.count (gwId) > 0)
            {
              switch ((*itPhy).second.outcomes.at (gwId))
                {
                case RECEIVED:
                  {
                    packetCounts.at (1)++;
                    break;
                  }
                case INTERFERED:
                  {
                    packetCounts.at (2)++;
                    break;
                  }
                case NO_MORE_RECEIVERS:
                  {
                    packetCounts.at (3)++;
                    break;
                  }
                case UNDER_SENSITIVITY:
                  {
                    packetCounts.at (4)++;
                    break;
                  }
                case LOST_BECAUSE_TX:
                  {
                    packetCounts.at (5)++;
                    break;
                  }
                case UNSET:
                  {
                    break;
                  }
                }
            }
        }
    }

  return packetCounts;
}
std::string
LoraPacketTracker::PrintPhyPacketsPerGw (Time startTime, Time stopTime,
                                         int gwId)
{
  // Vector packetCounts will contain - for the interval given in the input of
  // the function, the following fields: totPacketsSent receivedPackets
  // interferedPackets noMoreGwPackets underSensitivityPackets

  std::vector<int> packetCounts (6, 0);

  for (auto itPhy = m_packetTracker.begin ();
       itPhy != m_packetTracker.end ();
       ++itPhy)
    {
      if ((*itPhy).second.sendTime >= startTime && (*itPhy).second.sendTime <= stopTime)
        {
          packetCounts.at (0)++;

          NS_LOG_DEBUG ("Dealing with packet " << (*itPhy).second.packet);
          NS_LOG_DEBUG ("This packet was received by " <<
                        (*itPhy).second.outcomes.size () << " gateways");

          if((*itPhy).second.outcomes.find(gwId) != (*itPhy).second.outcomes.end())
            NS_LOG_DEBUG ("Packet outcome:" << (*itPhy).second.outcomes.at(gwId));


          if ((*itPhy).second.outcomes.count (gwId) > 0)
            {
              switch ((*itPhy).second.outcomes.at (gwId))
                {
                case RECEIVED:
                  {
                    packetCounts.at (1)++;
                    break;
                  }
                case INTERFERED:
                  {
                    packetCounts.at (2)++;
                    break;
                  }
                case NO_MORE_RECEIVERS:
                  {
                    packetCounts.at (3)++;
                    break;
                  }
                case UNDER_SENSITIVITY:
                  {
                    packetCounts.at (4)++;
                    break;
                  }
                case LOST_BECAUSE_TX:
                  {
                    packetCounts.at (5)++;
                    break;
                  }
                case UNSET:
                  {
                    break;
                  }
                }
            }
        }
    }

  std::string output ("");
  for (int i = 0; i < 6; ++i)
    {
      output += std::to_string (packetCounts.at (i)) + " ";
    }

  return output;
}

  std::string
  LoraPacketTracker::CountMacPacketsGlobally (Time startTime, Time stopTime)
  {
    NS_LOG_FUNCTION (this << startTime << stopTime);

    double sent = 0;
    double received = 0;
    for (auto it = m_macPacketTracker.begin ();
         it != m_macPacketTracker.end ();
         ++it)
      {
        if ((*it).second.sendTime >= startTime && (*it).second.sendTime <= stopTime)
          {
            sent++;
            if ((*it).second.receptionTimes.size ())
              {
                received++;
              }
          }
      }

 	  return std::to_string (sent) + " " +
      std::to_string (received) + " PDR = " + std::to_string((received/sent)*100);
  }

  // created by BC
  std::vector<double>
  LoraPacketTracker::returnInfoVector (Time startTime, Time stopTime)
  {
    NS_LOG_FUNCTION (this << startTime << stopTime);

    double sent = 0;
    double received = 0;
    for (auto it = m_macPacketTracker.begin ();
         it != m_macPacketTracker.end ();
         ++it)
      {
        if ((*it).second.sendTime >= startTime && (*it).second.sendTime <= stopTime)
          {
            sent++;
            if ((*it).second.receptionTimes.size ())
              {
                received++;
              }
          }
      }

    //modified by BC
    std::vector<double> output;
    output.push_back(sent);
    output.push_back(received);
    return output;
    		/*std::to_string (sent) + " " +
      std::to_string (received) + " PDR = " + std::to_string((received/sent)*100);*/
  }

  std::string
  LoraPacketTracker::CountMacPacketsGloballyCpsr (Time startTime, Time stopTime)
  {
    NS_LOG_FUNCTION (this << startTime << stopTime);

    double sent = 0;
    double received = 0;
    for (auto it = m_reTransmissionTracker.begin ();
         it != m_reTransmissionTracker.end ();
         ++it)
      {
        if ((*it).second.firstAttempt >= startTime && (*it).second.firstAttempt <= stopTime)
          {
            sent++;
            NS_LOG_DEBUG ("Found a packet");
            NS_LOG_DEBUG ("Number of attempts: " << unsigned(it->second.reTxAttempts) <<
                          ", successful: " << it->second.successful);
            if (it->second.successful)
              {
                received++;
              }
          }
      }

    return std::to_string (sent) + " " +
      std::to_string (received);
  }

}
}
