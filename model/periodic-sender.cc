/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
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

#include "ns3/periodic-sender.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/lora-net-device.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/class-a-end-device-lorawan-mac.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("PeriodicSender");

NS_OBJECT_ENSURE_REGISTERED (PeriodicSender);

TypeId
PeriodicSender::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::PeriodicSender")
    		.SetParent<Application> ()
			.AddConstructor<PeriodicSender> ()
			.SetGroupName ("lorawan")
			.AddAttribute ("Interval", "The interval between packet sends of this app",
					TimeValue (Seconds (0)),
					MakeTimeAccessor (&PeriodicSender::GetInterval,
							&PeriodicSender::SetInterval),
							MakeTimeChecker ());
	// .AddAttribute ("PacketSizeRandomVariable", "The random variable that determines the shape of the packet size, in bytes",
	//                StringValue ("ns3::UniformRandomVariable[Min=0,Max=10]"),
	//                MakePointerAccessor (&PeriodicSender::m_pktSizeRV),
	//                MakePointerChecker <RandomVariableStream>());
	return tid;
}

PeriodicSender::PeriodicSender ()
: m_interval (Seconds (10)),
  m_initialDelay (Seconds (1)),
  m_basePktSize (10),
  m_pktSizeRV (0)

{
	NS_LOG_FUNCTION_NOARGS ();
}

PeriodicSender::~PeriodicSender ()
{
	NS_LOG_FUNCTION_NOARGS ();
}

void
PeriodicSender::SetInterval (Time interval)
{
	NS_LOG_FUNCTION (this << interval);
	m_interval = interval;
}

Time
PeriodicSender::GetInterval (void) const
{
	NS_LOG_FUNCTION (this);
	return m_interval;
}

void
PeriodicSender::SetInitialDelay (Time delay)
{
	NS_LOG_FUNCTION (this << delay);
	m_initialDelay = delay;
}


void
PeriodicSender::SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv)
{
	m_pktSizeRV = rv;
}


void
PeriodicSender::SetPacketSize (uint8_t size)
{
	m_basePktSize = size;
}

//added by BC
void
PeriodicSender::SetRandomPeriodDelay (double randTimeDelay)
{
	m_randomTimeDelay = randTimeDelay;
}

void
PeriodicSender::SetVariableTX (bool varTx)
{
 m_isTxVariable = varTx;
}

void
PeriodicSender::SetTimedBehaviour (bool timedBeh)
{
 m_isBehaviourTimeBased = timedBeh;
}


void
PeriodicSender::SendPacket (void)
{
	NS_LOG_FUNCTION (this);

	// Create and send a new packet
	Ptr<Packet> packet;
	if (m_pktSizeRV)
	{
		int randomsize = m_pktSizeRV->GetInteger ();
		packet = Create<Packet> (m_basePktSize + randomsize);
	}
	else
	{
		packet = Create<Packet> (m_basePktSize);
	}
	m_mac->Send (packet);


	// here need to check the current SF of the sender, update the tx time if needed and also create
	// the 12 hours shifting behaviour TODO
	Time newInterval = m_interval;
	int switchBehaviourFlag = 0;
	double randomDelay = 0;

	if(m_isBehaviourTimeBased == true){
		//std::cout<< "Times behaviour is true" << std::endl;
		double timesPerBehaviour[2] = {630,7462}; // 500, 7200 in seconds
		//std::cout << timesPerBehaviour[0] << "," << timesPerBehaviour[1] << std::endl;
		if (Simulator::Now() - m_lastTimeSwitched > Seconds(43200)){ // 43200
			m_lastTimeSwitched = Simulator::Now();
			switchBehaviourFlag = 1;
		}

		// switch behaviour if needed
		if (switchBehaviourFlag == 1){
			//std::cout<< "Switching behaviour now" << std::endl;
			if (m_currentBehaviour == 0){
				m_currentBehaviour = 1;
			}
			else if (m_currentBehaviour == 1){
				m_currentBehaviour = 0;
			}
			switchBehaviourFlag = 0; // reset flag
		}

		newInterval = Seconds(timesPerBehaviour[m_currentBehaviour]);
		//std::cout << "Set new Interval to " << newInterval << ", " << timesPerBehaviour[m_currentBehaviour]<<std::endl;
	}

	if (m_isTxVariable == true){ // for group2
	//std::cout<< "TxVariable is true" << std::endl;
    Ptr<LoraNetDevice> loraNetDevice = m_node->GetDevice (0)->GetObject<LoraNetDevice> ();

	Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac()->GetObject<
			ClassAEndDeviceLorawanMac>();
    int sf = mac->GetDataRate(); // get current sf

    //double timesPerSF[6] = {1.483,0.824,0.412,0.206,0.114,0.062}; // how it should be
    double timesPerSF[6] = {0.9106,0.5249,0.2722,0.1928,0.1212,0.1212};  // hardcoded from deployment
    newInterval = Seconds(timesPerSF[sf]*99);
    if (m_randomTimeDelay != 0) // this is for group 4
    	{
    		m_randomTimeDelay = timesPerSF[sf]*99; // this is for group 4, if there is a random time delay, make it the same as new TxInterval
    		randomDelay = 0 + (double)rand() / RAND_MAX * (m_randomTimeDelay*2 - 0);
    		newInterval = newInterval+Seconds(randomDelay);
    	}
	}

	// for group 3
	if ((m_randomTimeDelay != 0)&&(m_isTxVariable == false)) {

		randomDelay = 0 + (double)rand() / RAND_MAX * (m_randomTimeDelay*2 - 0);
		newInterval = m_interval+Seconds(randomDelay);
	}


	m_sendEvent = Simulator::Schedule (newInterval, &PeriodicSender::SendPacket, this);
	NS_LOG_DEBUG ("FINALLY Sent a packet of size " << packet->GetSize ());

	// changed by BC
	//WAS
	// // Schedule the next SendPacket event
	//m_sendEvent = Simulator::Schedule (m_interval, &PeriodicSender::SendPacket,
	//                                   this);

	// this will execute even if packet is NOT sent because of duty cycle, so taking log here is not good, will return ALL scheduled packets

}

void
PeriodicSender::StartApplication (void)
{
	NS_LOG_FUNCTION (this);

	// Make sure we have a MAC layer
	if (m_mac == 0)
	{
		// Assumes there's only one device
		Ptr<LoraNetDevice> loraNetDevice = m_node->GetDevice (0)->GetObject<LoraNetDevice> ();

		m_mac = loraNetDevice->GetMac ();
		NS_ASSERT (m_mac != 0);
	}

	// Schedule the next SendPacket event
	Simulator::Cancel (m_sendEvent);
	NS_LOG_DEBUG ("Starting up application with a first event with a " <<
			m_initialDelay.GetSeconds () << " seconds delay");
	m_sendEvent = Simulator::Schedule (m_initialDelay,
			&PeriodicSender::SendPacket, this);
	NS_LOG_DEBUG ("Event Id: " << m_sendEvent.GetUid ());
}

void
PeriodicSender::StopApplication (void)
{
	NS_LOG_FUNCTION_NOARGS ();
	Simulator::Cancel (m_sendEvent);
}

}
}
