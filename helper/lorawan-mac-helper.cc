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

#include "ns3/lorawan-mac-helper.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/lora-net-device.h"
#include "ns3/log.h"
#include "ns3/random-variable-stream.h"

//added by BC
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
/*
namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("LorawanMacHelper");

LorawanMacHelper::LorawanMacHelper () : m_region (LorawanMacHelper::EU)
{
}

void
LorawanMacHelper::Set (std::string name, const AttributeValue &v)
{
  m_mac.Set (name, v);
}

void
LorawanMacHelper::SetDeviceType (enum DeviceType dt)
{
  NS_LOG_FUNCTION (this << dt);
  switch (dt)
    {
    case GW:
      m_mac.SetTypeId ("ns3::GatewayLorawanMac");
      break;
    case ED_A:
      m_mac.SetTypeId ("ns3::ClassAEndDeviceLorawanMac");
      break;
    }
  m_deviceType = dt;
}

void
LorawanMacHelper::SetAddressGenerator (Ptr<LoraDeviceAddressGenerator> addrGen)
{
  NS_LOG_FUNCTION (this);

  m_addrGen = addrGen;
}

void
LorawanMacHelper::SetRegion (enum LorawanMacHelper::Regions region)
{
  m_region = region;
}

Ptr<LorawanMac>
LorawanMacHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
  Ptr<LorawanMac> mac = m_mac.Create<LorawanMac> ();
  mac->SetDevice (device);

  // If we are operating on an end device, add an address to it
  if (m_deviceType == ED_A && m_addrGen != 0)
    {
      mac->GetObject<ClassAEndDeviceLorawanMac> ()->SetDeviceAddress (m_addrGen->NextAddress ());
    }

  // Add a basic list of channels based on the region where the device is
  // operating
  if (m_deviceType == ED_A)
    {
      Ptr<ClassAEndDeviceLorawanMac> edMac = mac->GetObject<ClassAEndDeviceLorawanMac> ();
      switch (m_region)
        {
        case LorawanMacHelper::EU:
          {
            ConfigureForEuRegion (edMac);
            break;
          }
        case LorawanMacHelper::ALOHA:
          {
            ConfigureForAlohaRegion (edMac);
            break;
          }
        default:
          {
            NS_LOG_ERROR ("This region isn't supported yet!");
            break;
          }
        }
    }
  else
    {
      Ptr<GatewayLorawanMac> gwMac = mac->GetObject<GatewayLorawanMac> ();
      switch (m_region)
        {
        case LorawanMacHelper::EU:
          {
            ConfigureForEuRegion (gwMac);
            break;
          }
        case LorawanMacHelper::ALOHA:
          {
            ConfigureForAlohaRegion (gwMac);
            break;
          }
        default:
          {
            NS_LOG_ERROR ("This region isn't supported yet!");
            break;
          }
        }
    }
  return mac;
}

void
LorawanMacHelper::ConfigureForAlohaRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ApplyCommonAlohaConfigurations (edMac);

  /////////////////////////////////////////////////////
  // TxPower -> Transmission power in dBm conversion //
  /////////////////////////////////////////////////////
  edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

  ////////////////////////////////////////////////////////////
  // Matrix to know which DataRate the GW will respond with //
  ////////////////////////////////////////////////////////////
  LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
                                             {{1, 0, 0, 0, 0, 0}},
                                             {{2, 1, 0, 0, 0, 0}},
                                             {{3, 2, 1, 0, 0, 0}},
                                             {{4, 3, 2, 1, 0, 0}},
                                             {{5, 4, 3, 2, 1, 0}},
                                             {{6, 5, 4, 3, 2, 1}},
                                             {{7, 6, 5, 4, 3, 2}}}};
  edMac->SetReplyDataRateMatrix (matrix);

  /////////////////////
  // Preamble length //
  /////////////////////
  edMac->SetNPreambleSymbols (8);

  //////////////////////////////////////
  // Second receive window parameters //
  //////////////////////////////////////
  edMac->SetSecondReceiveWindowDataRate (0);
  edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForAlohaRegion (Ptr<GatewayLorawanMac> gwMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ///////////////////////////////
  // ReceivePath configuration //
  ///////////////////////////////
  Ptr<GatewayLoraPhy> gwPhy =
      gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

  ApplyCommonEuConfigurations (gwMac);

  if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
    {
      NS_LOG_DEBUG ("Resetting reception paths");
      gwPhy->ResetReceptionPaths ();

      std::vector<double> frequencies;
      frequencies.push_back (868.1);

      std::vector<double>::iterator it = frequencies.begin ();

      int receptionPaths = 0;
      int maxReceptionPaths = 1;
      while (receptionPaths < maxReceptionPaths)
        {
          if (it == frequencies.end ())
            {
              it = frequencies.begin ();
            }
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath (*it);
          ++it;
          receptionPaths++;
        }
    }
}

void
LorawanMacHelper::ApplyCommonAlohaConfigurations (Ptr<LorawanMac> lorawanMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  //////////////
  // SubBands //
  //////////////

  LogicalLoraChannelHelper channelHelper;
  channelHelper.AddSubBand (868, 868.6, 1, 14);

  //////////////////////
  // Default channels //
  //////////////////////
  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
  channelHelper.AddChannel (lc1);

  lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

  ///////////////////////////////////////////////
  // DataRate -> SF, DataRate -> Bandwidth     //
  // and DataRate -> MaxAppPayload conversions //
  ///////////////////////////////////////////////
  lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
  lorawanMac->SetBandwidthForDataRate (
      std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
  lorawanMac->SetMaxAppPayloadForDataRate (
      std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}

void
LorawanMacHelper::ConfigureForEuRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ApplyCommonEuConfigurations (edMac);

  /////////////////////////////////////////////////////
  // TxPower -> Transmission power in dBm conversion //
  /////////////////////////////////////////////////////
  edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

  ////////////////////////////////////////////////////////////
  // Matrix to know which DataRate the GW will respond with //
  ////////////////////////////////////////////////////////////
  LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
                                             {{1, 0, 0, 0, 0, 0}},
                                             {{2, 1, 0, 0, 0, 0}},
                                             {{3, 2, 1, 0, 0, 0}},
                                             {{4, 3, 2, 1, 0, 0}},
                                             {{5, 4, 3, 2, 1, 0}},
                                             {{6, 5, 4, 3, 2, 1}},
                                             {{7, 6, 5, 4, 3, 2}}}};
  edMac->SetReplyDataRateMatrix (matrix);

  /////////////////////
  // Preamble length //
  /////////////////////
  edMac->SetNPreambleSymbols (8);

  //////////////////////////////////////
  // Second receive window parameters //
  //////////////////////////////////////
  edMac->SetSecondReceiveWindowDataRate (0);
  edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForEuRegion (Ptr<GatewayLorawanMac> gwMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  ///////////////////////////////
  // ReceivePath configuration //
  ///////////////////////////////
  Ptr<GatewayLoraPhy> gwPhy =
      gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

  ApplyCommonEuConfigurations (gwMac);

  if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
    {
      NS_LOG_DEBUG ("Resetting reception paths");
      gwPhy->ResetReceptionPaths ();

      std::vector<double> frequencies;
      frequencies.push_back (868.1);
      frequencies.push_back (868.3);
      frequencies.push_back (868.5);
      // Added by BC
      frequencies.push_back (867.1);
      frequencies.push_back (867.3);
      frequencies.push_back (867.5);
      frequencies.push_back (867.7);
      frequencies.push_back (867.9);

      std::vector<double>::iterator it = frequencies.begin ();

      int receptionPaths = 0;
      int maxReceptionPaths = 8;
      while (receptionPaths < maxReceptionPaths)
        {
          if (it == frequencies.end ())
            {
              it = frequencies.begin ();
            }
          gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath (*it);
          ++it;
          receptionPaths++;
        }
    }
}

void
LorawanMacHelper::ApplyCommonEuConfigurations (Ptr<LorawanMac> lorawanMac) const
{
  NS_LOG_FUNCTION_NOARGS ();

  //////////////
  // SubBands //
  //////////////

  g (863.0 – 868.0 MHz): 1%
  g1 (868.0 – 868.6 MHz): 1%
  g2 (868.7 – 869.2 MHz): 0.1%


  LogicalLoraChannelHelper channelHelper;
  channelHelper.AddSubBand (868.0, 868.6, 0.01, 14);
  channelHelper.AddSubBand (868.7, 869.2, 0.001, 14);
  channelHelper.AddSubBand (869.4, 869.65, 0.1, 27);

  //Added by BC
  //channelHelper.AddSubBand (863.0, 868.0, 0.01, 14);

  //////////////////////
  // Default channels //
  //////////////////////
  Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
  Ptr<LogicalLoraChannel> lc2 = CreateObject<LogicalLoraChannel> (868.3, 0, 5);
  Ptr<LogicalLoraChannel> lc3 = CreateObject<LogicalLoraChannel> (868.5, 0, 5);

  //Added by BC

  Ptr<LogicalLoraChannel> lc4 = CreateObject<LogicalLoraChannel> (867.1, 0, 5);
  Ptr<LogicalLoraChannel> lc5 = CreateObject<LogicalLoraChannel> (867.3, 0, 5);
  Ptr<LogicalLoraChannel> lc6 = CreateObject<LogicalLoraChannel> (867.5, 0, 5);
  Ptr<LogicalLoraChannel> lc7 = CreateObject<LogicalLoraChannel> (867.7, 0, 5);
  Ptr<LogicalLoraChannel> lc8 = CreateObject<LogicalLoraChannel> (867.9, 0, 5);

  channelHelper.AddChannel (lc1);
  channelHelper.AddChannel (lc2);
  channelHelper.AddChannel (lc3);
  //Added by BC

  channelHelper.AddChannel (lc4);
  channelHelper.AddChannel (lc5);
  channelHelper.AddChannel (lc6);
  channelHelper.AddChannel (lc7);
  channelHelper.AddChannel (lc8);


  lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

  ///////////////////////////////////////////////
  // DataRate -> SF, DataRate -> Bandwidth     //
  // and DataRate -> MaxAppPayload conversions //
  ///////////////////////////////////////////////
  lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
  lorawanMac->SetBandwidthForDataRate (
      std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
  lorawanMac->SetMaxAppPayloadForDataRate (
      std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}
 */



namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("LorawanMacHelper");

LorawanMacHelper::LorawanMacHelper () : m_region (LorawanMacHelper::EU)
{
}

void
LorawanMacHelper::Set (std::string name, const AttributeValue &v)
{
	m_mac.Set (name, v);
}

void
LorawanMacHelper::SetDeviceType (enum DeviceType dt)
{
	NS_LOG_FUNCTION (this << dt);
	switch (dt)
	{
	case GW:
		m_mac.SetTypeId ("ns3::GatewayLorawanMac");
		break;
	case ED_A:
		m_mac.SetTypeId ("ns3::ClassAEndDeviceLorawanMac");
		break;
	}
	m_deviceType = dt;
}

void
LorawanMacHelper::SetAddressGenerator (Ptr<LoraDeviceAddressGenerator> addrGen)
{
	NS_LOG_FUNCTION (this);

	m_addrGen = addrGen;
}

void
LorawanMacHelper::SetRegion (enum LorawanMacHelper::Regions region)
{
	m_region = region;
}

Ptr<LorawanMac>
LorawanMacHelper::Create (Ptr<Node> node, Ptr<NetDevice> device) const
{
	Ptr<LorawanMac> mac = m_mac.Create<LorawanMac> ();
	mac->SetDevice (device);

	// If we are operating on an end device, add an address to it
	if (m_deviceType == ED_A && m_addrGen != 0)
	{
		mac->GetObject<ClassAEndDeviceLorawanMac> ()->SetDeviceAddress (m_addrGen->NextAddress ());
	}

	// Add a basic list of channels based on the region where the device is
	// operating
	if (m_deviceType == ED_A)
	{
		Ptr<ClassAEndDeviceLorawanMac> edMac = mac->GetObject<ClassAEndDeviceLorawanMac> ();
		switch (m_region)
		{
		case LorawanMacHelper::EU: {
			ConfigureForEuRegion (edMac);
			break;
		}
		case LorawanMacHelper::SingleChannel: {
			ConfigureForSingleChannelRegion (edMac);
			break;
		}
		case LorawanMacHelper::ALOHA: {
			ConfigureForAlohaRegion (edMac);
			break;
		}
		default: {
			NS_LOG_ERROR ("This region isn't supported yet!");
			break;
		}
		}
	}
	else
	{
		Ptr<GatewayLorawanMac> gwMac = mac->GetObject<GatewayLorawanMac> ();
		switch (m_region)
		{
		case LorawanMacHelper::EU: {
			ConfigureForEuRegion (gwMac);
			break;
		}
		case LorawanMacHelper::SingleChannel: {
			ConfigureForSingleChannelRegion (gwMac);
			break;
		}
		case LorawanMacHelper::ALOHA: {
			ConfigureForAlohaRegion (gwMac);
			break;
		}
		default: {
			NS_LOG_ERROR ("This region isn't supported yet!");
			break;
		}
		}
	}
	return mac;
}

void
LorawanMacHelper::ConfigureForAlohaRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	ApplyCommonAlohaConfigurations (edMac);

	/////////////////////////////////////////////////////
	// TxPower -> Transmission power in dBm conversion //
	/////////////////////////////////////////////////////
	edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

	////////////////////////////////////////////////////////////
	// Matrix to know which DataRate the GW will respond with //
	////////////////////////////////////////////////////////////
	LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
			{{1, 0, 0, 0, 0, 0}},
			{{2, 1, 0, 0, 0, 0}},
			{{3, 2, 1, 0, 0, 0}},
			{{4, 3, 2, 1, 0, 0}},
			{{5, 4, 3, 2, 1, 0}},
			{{6, 5, 4, 3, 2, 1}},
			{{7, 6, 5, 4, 3, 2}}}};
	edMac->SetReplyDataRateMatrix (matrix);

	/////////////////////
	// Preamble length //
	/////////////////////
	edMac->SetNPreambleSymbols (8);

	//////////////////////////////////////
	// Second receive window parameters //
	//////////////////////////////////////
	edMac->SetSecondReceiveWindowDataRate (0);
	edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForAlohaRegion (Ptr<GatewayLorawanMac> gwMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	///////////////////////////////
	// ReceivePath configuration //
	///////////////////////////////
	Ptr<GatewayLoraPhy> gwPhy =
			gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

	ApplyCommonAlohaConfigurations (gwMac);

	if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
	{
		NS_LOG_DEBUG ("Resetting reception paths");
		gwPhy->ResetReceptionPaths ();

		int receptionPaths = 0;
		int maxReceptionPaths = 1;
		while (receptionPaths < maxReceptionPaths)
		{
			gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath ();
			receptionPaths++;
		}
		gwPhy->AddFrequency (868.1);
	}
}

void
LorawanMacHelper::ApplyCommonAlohaConfigurations (Ptr<LorawanMac> lorawanMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	//////////////
	// SubBands //
	//////////////

	LogicalLoraChannelHelper channelHelper;
	channelHelper.AddSubBand (868, 868.6, 1, 14);

	//////////////////////
	// Default channels //
	//////////////////////
	Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
	channelHelper.AddChannel (lc1);

	lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

	///////////////////////////////////////////////
	// DataRate -> SF, DataRate -> Bandwidth     //
	// and DataRate -> MaxAppPayload conversions //
	///////////////////////////////////////////////
	lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
	lorawanMac->SetBandwidthForDataRate (
			std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
	lorawanMac->SetMaxAppPayloadForDataRate (
			std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}

void
LorawanMacHelper::ConfigureForEuRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	ApplyCommonEuConfigurations (edMac);

	/////////////////////////////////////////////////////
	// TxPower -> Transmission power in dBm conversion //
	/////////////////////////////////////////////////////
	edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

	////////////////////////////////////////////////////////////
	// Matrix to know which DataRate the GW will respond with //
	////////////////////////////////////////////////////////////
	LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
			{{1, 0, 0, 0, 0, 0}},
			{{2, 1, 0, 0, 0, 0}},
			{{3, 2, 1, 0, 0, 0}},
			{{4, 3, 2, 1, 0, 0}},
			{{5, 4, 3, 2, 1, 0}},
			{{6, 5, 4, 3, 2, 1}},
			{{7, 6, 5, 4, 3, 2}}}};
	edMac->SetReplyDataRateMatrix (matrix);

	/////////////////////
	// Preamble length //
	/////////////////////
	edMac->SetNPreambleSymbols (8);

	//////////////////////////////////////
	// Second receive window parameters //
	//////////////////////////////////////
	edMac->SetSecondReceiveWindowDataRate (0);
	edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForEuRegion (Ptr<GatewayLorawanMac> gwMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	///////////////////////////////
	// ReceivePath configuration //
	///////////////////////////////
	Ptr<GatewayLoraPhy> gwPhy =
			gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

	ApplyCommonEuConfigurations (gwMac);

	if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
	{
		NS_LOG_DEBUG ("Resetting reception paths");
		gwPhy->ResetReceptionPaths ();

		std::vector<double> frequencies;
		frequencies.push_back (868.1);
		frequencies.push_back (868.3);
		frequencies.push_back (868.5);
		// Added by BC
		frequencies.push_back (867.1);
		frequencies.push_back (867.3);
		frequencies.push_back (867.5);
		frequencies.push_back (867.7);
		frequencies.push_back (867.9);

		for (auto &f : frequencies)
		{
			gwPhy->AddFrequency (f);
		}

		int receptionPaths = 0;
		int maxReceptionPaths = 8; // TODO SWITCH THIS BACK
		while (receptionPaths < maxReceptionPaths)
		{
			gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath ();
			receptionPaths++;
		}
	}
}

void
LorawanMacHelper::ApplyCommonEuConfigurations (Ptr<LorawanMac> lorawanMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	//////////////
	// SubBands //
	//////////////

	//g (863.0 – 868.0 MHz): 1%
	//g1 (868.0 – 868.6 MHz): 1%
	//g2 (868.7 – 869.2 MHz): 0.1%


	LogicalLoraChannelHelper channelHelper;
	channelHelper.AddSubBand (868.0, 868.6, 0.01, 14);
	channelHelper.AddSubBand (868.7, 869.2, 0.001, 14);
	channelHelper.AddSubBand (869.4, 869.65, 0.1, 27);

	//Added by BC
	channelHelper.AddSubBand (863.0, 868.0, 0.01, 14);

	//////////////////////
	// Default channels //
	//////////////////////
	Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
	Ptr<LogicalLoraChannel> lc2 = CreateObject<LogicalLoraChannel> (868.3, 0, 5);
	Ptr<LogicalLoraChannel> lc3 = CreateObject<LogicalLoraChannel> (868.5, 0, 5);

	//Added by BC
	Ptr<LogicalLoraChannel> lc4 = CreateObject<LogicalLoraChannel> (867.1, 0, 5);
	Ptr<LogicalLoraChannel> lc5 = CreateObject<LogicalLoraChannel> (867.3, 0, 5);
	Ptr<LogicalLoraChannel> lc6 = CreateObject<LogicalLoraChannel> (867.5, 0, 5);
	Ptr<LogicalLoraChannel> lc7 = CreateObject<LogicalLoraChannel> (867.7, 0, 5);
	Ptr<LogicalLoraChannel> lc8 = CreateObject<LogicalLoraChannel> (867.9, 0, 5);

	channelHelper.AddChannel (lc1);
	channelHelper.AddChannel (lc2);
	channelHelper.AddChannel (lc3);

	//Added by BC
	channelHelper.AddChannel (lc4);
	channelHelper.AddChannel (lc5);
	channelHelper.AddChannel (lc6);
	channelHelper.AddChannel (lc7);
	channelHelper.AddChannel (lc8);


	lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

	///////////////////////////////////////////////
	// DataRate -> SF, DataRate -> Bandwidth     //
	// and DataRate -> MaxAppPayload conversions //
	///////////////////////////////////////////////
	lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
	lorawanMac->SetBandwidthForDataRate (
			std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
	lorawanMac->SetMaxAppPayloadForDataRate (
			std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}

///////////////////////////////

void
LorawanMacHelper::ConfigureForSingleChannelRegion (Ptr<ClassAEndDeviceLorawanMac> edMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	ApplyCommonSingleChannelConfigurations (edMac);

	/////////////////////////////////////////////////////
	// TxPower -> Transmission power in dBm conversion //
	/////////////////////////////////////////////////////
	edMac->SetTxDbmForTxPower (std::vector<double>{16, 14, 12, 10, 8, 6, 4, 2});

	////////////////////////////////////////////////////////////
	// Matrix to know which DataRate the GW will respond with //
	////////////////////////////////////////////////////////////
	LorawanMac::ReplyDataRateMatrix matrix = {{{{0, 0, 0, 0, 0, 0}},
			{{1, 0, 0, 0, 0, 0}},
			{{2, 1, 0, 0, 0, 0}},
			{{3, 2, 1, 0, 0, 0}},
			{{4, 3, 2, 1, 0, 0}},
			{{5, 4, 3, 2, 1, 0}},
			{{6, 5, 4, 3, 2, 1}},
			{{7, 6, 5, 4, 3, 2}}}};
	edMac->SetReplyDataRateMatrix (matrix);

	/////////////////////
	// Preamble length //
	/////////////////////
	edMac->SetNPreambleSymbols (8);

	//////////////////////////////////////
	// Second receive window parameters //
	//////////////////////////////////////
	edMac->SetSecondReceiveWindowDataRate (0);
	edMac->SetSecondReceiveWindowFrequency (869.525);
}

void
LorawanMacHelper::ConfigureForSingleChannelRegion (Ptr<GatewayLorawanMac> gwMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	///////////////////////////////
	// ReceivePath configuration //
	///////////////////////////////
	Ptr<GatewayLoraPhy> gwPhy =
			gwMac->GetDevice ()->GetObject<LoraNetDevice> ()->GetPhy ()->GetObject<GatewayLoraPhy> ();

	ApplyCommonEuConfigurations (gwMac);

	if (gwPhy) // If cast is successful, there's a GatewayLoraPhy
	{
		NS_LOG_DEBUG ("Resetting reception paths");
		gwPhy->ResetReceptionPaths ();

		std::vector<double> frequencies;
		frequencies.push_back (863);

		for (auto &f : frequencies)
		{
			gwPhy->AddFrequency (f);
		}

		// changed by BC to feature infinite parallel demodulation paths
		int receptionPaths = 0;
		int maxReceptionPaths = 10000;
		while (receptionPaths < maxReceptionPaths)
		{
			gwPhy->GetObject<GatewayLoraPhy> ()->AddReceptionPath ();
			receptionPaths++;
		}
	}
}

void
LorawanMacHelper::ApplyCommonSingleChannelConfigurations (Ptr<LorawanMac> lorawanMac) const
{
	NS_LOG_FUNCTION_NOARGS ();

	//////////////
	// SubBands //
	//////////////

	LogicalLoraChannelHelper channelHelper;
	channelHelper.AddSubBand (868.0, 869.0, 0.01, 14);

	//////////////////////
	// Default channels //
	//////////////////////
	Ptr<LogicalLoraChannel> lc1 = CreateObject<LogicalLoraChannel> (868.1, 0, 5);
	channelHelper.AddChannel (lc1);

	lorawanMac->SetLogicalLoraChannelHelper (channelHelper);

	///////////////////////////////////////////////
	// DataRate -> SF, DataRate -> Bandwidth     //
	// and DataRate -> MaxAppPayload conversions //
	///////////////////////////////////////////////
	lorawanMac->SetSfForDataRate (std::vector<uint8_t>{12, 11, 10, 9, 8, 7, 7});
	lorawanMac->SetBandwidthForDataRate (
			std::vector<double>{125000, 125000, 125000, 125000, 125000, 125000, 250000});
	lorawanMac->SetMaxAppPayloadForDataRate (
			std::vector<uint32_t>{59, 59, 59, 123, 230, 230, 230, 230});
}


std::vector<int>
LorawanMacHelper::SetSpreadingFactorsUp (NodeContainer endDevices, NodeContainer gateways,
		Ptr<LoraChannel> channel)
{
	NS_LOG_FUNCTION_NOARGS ();

	std::vector<int> sfQuantity (7, 0);
	for (NodeContainer::Iterator j = endDevices.Begin(); j != endDevices.End(); ++j)
	{
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT(position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice (0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT(loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
		NS_ASSERT(mac != 0);

		// Try computing the distance from each gateway and find the best one
		Ptr<Node> bestGateway = gateways.Get(0);
		Ptr<MobilityModel> bestGatewayPosition = bestGateway->GetObject<MobilityModel> ();

		// Assume devices transmit at 14 dBm
		double highestRxPower = channel->GetRxPower(14, position, bestGatewayPosition);

		for (NodeContainer::Iterator currentGw = gateways.Begin () + 1; currentGw != gateways.End ();
				++currentGw)
		{
			// Compute the power received from the current gateway
			Ptr<Node> curr = *currentGw;
			Ptr<MobilityModel> currPosition = curr->GetObject<MobilityModel> ();
			double currentRxPower = channel->GetRxPower (14, position, currPosition); // dBm

			if (currentRxPower > highestRxPower)
			{
				bestGateway = curr;
				bestGatewayPosition = curr->GetObject<MobilityModel> ();
				highestRxPower = currentRxPower;
			}
		}

		// NS_LOG_DEBUG ("Rx Power: " << highestRxPower);
		double rxPower = highestRxPower;

		/*
      // Get the ED sensitivity
      Ptr<EndDeviceLoraPhy> edPhy = loraNetDevice->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
      const double *edSensitivity = edPhy->sensitivity;

      if (rxPower > *edSensitivity)
        {
          mac->SetDataRate(5);
          sfQuantity[0] = sfQuantity[0] + 1;
        }
      else if (rxPower > *(edSensitivity + 1))
        {
          mac->SetDataRate(4);
          sfQuantity[1] = sfQuantity[1] + 1;
        }
      else if (rxPower > *(edSensitivity + 2))
        {
          mac->SetDataRate(3);
          sfQuantity[2] = sfQuantity[2] + 1;
        }
      else if (rxPower > *(edSensitivity + 3))
        {
          mac->SetDataRate(2);
          sfQuantity[3] = sfQuantity[3] + 1;
        }
      else if (rxPower > *(edSensitivity + 4))
        {
          mac->SetDataRate(1);
          sfQuantity[4] = sfQuantity[4] + 1;
        }
      else if (rxPower > *(edSensitivity + 5))
        {
          mac->SetDataRate(0);
          sfQuantity[5] = sfQuantity[5] + 1;
        }
      else // Device is out of range. Assign SF12.
        {
          // NS_LOG_DEBUG ("Device out of range");
          mac->SetDataRate (0);
          sfQuantity[6] = sfQuantity[6] + 1;
          // NS_LOG_DEBUG ("sfQuantity[6] = " << sfQuantity[6]);
        }
      //Added by BC (display Sf of node in LOG)
      Ptr<EndDeviceLorawanMac> mac2 = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();
      int sf = mac2 -> GetDataRate();
      NS_LOG_DEBUG ( "sf is, within lorawanMacHelper: " << sf );
		 */



		// Get the Gw sensitivity
		Ptr<NetDevice> gatewayNetDevice = bestGateway->GetDevice (0);
		Ptr<LoraNetDevice> gatewayLoraNetDevice = gatewayNetDevice->GetObject<LoraNetDevice> ();
		Ptr<GatewayLoraPhy> gatewayPhy = gatewayLoraNetDevice->GetPhy ()->GetObject<GatewayLoraPhy> ();
		const double *gwSensitivity = gatewayPhy->sensitivity;

		if(rxPower > *gwSensitivity)
		{
			mac->SetDataRate (5);
			sfQuantity[0] = sfQuantity[0] + 1;

		}
		else if (rxPower > *(gwSensitivity+1))
		{
			mac->SetDataRate (4);
			sfQuantity[1] = sfQuantity[1] + 1;

		}
		else if (rxPower > *(gwSensitivity+2))
		{
			mac->SetDataRate (3);
			sfQuantity[2] = sfQuantity[2] + 1;

		}
		else if (rxPower > *(gwSensitivity+3))
		{
			mac->SetDataRate (2);
			sfQuantity[3] = sfQuantity[3] + 1;
		}
		else if (rxPower > *(gwSensitivity+4))
		{
			mac->SetDataRate (1);
			sfQuantity[4] = sfQuantity[4] + 1;
		}
		else if (rxPower > *(gwSensitivity+5))
		{
			mac->SetDataRate (0);
			sfQuantity[5] = sfQuantity[5] + 1;

		}
		else // Device is out of range. Assign SF12.
		{
			mac->SetDataRate (0);
			sfQuantity[6] = sfQuantity[6] + 1;

		}


	} // end loop on nodes


	return sfQuantity;

} //  end function

// Function added by BC
std::vector<int>
LorawanMacHelper::SetAllSpreadingFactorsToX (NodeContainer endDevices, NodeContainer gateways,
		Ptr<LoraChannel> channel, int SF)
{
	NS_LOG_FUNCTION_NOARGS ();

	std::vector<int> sfQuantity (7, 0);
	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
		NS_ASSERT (mac != 0);
		Ptr<EndDeviceLorawanMac> mac2 = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();

		int adjSF = 12-SF;
		// set to SF7
		mac ->SetDataRate(adjSF);

		// Keeps track in sfQuantity of how many devices have which SF
		// 1 0 0 0 0 0 0 is 1 device with SF7, 0 0 0 0 0 0 1 is 1 device with SF 12
		switch (adjSF) {
		case 5:
			sfQuantity[0] = sfQuantity[0] + 1;
			break;
		case 4:
			sfQuantity[1] = sfQuantity[1] + 1;
			break;
		case 3:
			sfQuantity[2] = sfQuantity[2] + 1;
			break;
		case 2:
			sfQuantity[3] = sfQuantity[3] + 1;
			break;
		case 1:
			sfQuantity[4] = sfQuantity[4] + 1;
			break;
		case 0:
			sfQuantity[5] = sfQuantity[5] + 1;
			break;
		}

		int sf = mac2 -> GetDataRate();
		NS_LOG_DEBUG ( "sf is, within lorawanMacHelper: " << sf );
	} // end loop on nodes

	return sfQuantity;

} //  end function

// Function added by BC
int
LorawanMacHelper::SetSpreadingFactorToX (NodeContainer endDevices, NodeContainer gateways,
		Ptr<LoraChannel> channel, int node_id, int SF)
{
	NS_LOG_FUNCTION_NOARGS ();
	int assignedSF = SF;
	Ptr<Node> object = endDevices.Get (node_id);
	Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
	NS_ASSERT (position != 0);
	Ptr<NetDevice> netDevice = object->GetDevice(0);
	Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
	NS_ASSERT (loraNetDevice != 0);
	Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
	NS_ASSERT (mac != 0);
	Ptr<EndDeviceLorawanMac> mac2 = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();

	int adjSF = 12-assignedSF;
	// set to SF7
	mac ->SetDataRate(adjSF);

	return 12-mac2->GetDataRate();

} //  end function

// Function added by BC
int
LorawanMacHelper::SetDistributedSpreadingFactorsUpwards (NodeContainer endDevices, NodeContainer gateways, Ptr<LoraChannel> channel, std::vector<int> SFdistribution)
{
	//std::cout<<"Start"<<std::endl;
	std::vector<double> distanceVector;
	std::vector<double> distanceVectorSorted;
	std::vector<int> availableSFsVector;
	std::vector<int> SFtoAssignVector = SFdistribution;

	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		//		    std::cout<<"Inside first loop"<<std::endl;
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();

		int elementIndex = 0;
		std::vector<double> relativeDistances;

		//std::cout << "Before loop" <<std::endl;
		for (NodeContainer::Iterator i = gateways.Begin (); i != gateways.End (); ++i) {
			//std::cout << "loop starts" <<std::endl;
			// computing the distance from each gateway to find the best/worst one
			Ptr<Node> currGateway = *i;
			Ptr<MobilityModel> currGatewayPosition = currGateway->GetObject<MobilityModel> ();

			relativeDistances.push_back(position->GetDistanceFrom (currGatewayPosition));

			/*
				// print distances
				for (size_t i=0;i!=relativeDistances.size();i++){
					std::cout << "Relative Distances: " << relativeDistances[i] <<std::endl;
				}
			 */

			elementIndex = std::min_element(relativeDistances.begin(),relativeDistances.end()) - relativeDistances.begin();
			//std::cout << "Index: " << elementIndex <<std::endl;
		}

		//Ptr<Node> bestGateway = gateways.Get(elementIndex);
		Ptr<MobilityModel> bestGatewayPosition = gateways.Get(elementIndex)->GetObject<MobilityModel> ();
		distanceVector.push_back(position->GetDistanceFrom (bestGatewayPosition));
		// Assume devices transmit at 14 dBm
		double highestRxPower = channel->GetRxPower (14, position, bestGatewayPosition);

		// Get sensitivity values
		Ptr<EndDeviceLoraPhy> edPhy = loraNetDevice->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
		const double *edSensitivity = edPhy->sensitivity;

		//			std::cout<< "Distance:" << position->GetDistanceFrom (bestGatewayPosition) << ", RxPower: " << highestRxPower <<std::endl;
		if (highestRxPower > *edSensitivity)
		{
			// can take any sf
			//			  std::cout<< "Sensitivity:" << *edSensitivity << " push_back 5" <<std::endl;
			availableSFsVector.push_back(5);
		}
		else if (highestRxPower > *(edSensitivity + 1))
		{
			// can take 8-12 sf
			//			  std::cout<< "Sensitivity:" << *(edSensitivity +1)<< " push_back 4" <<std::endl;
			availableSFsVector.push_back(4);
		}
		else if (highestRxPower > *(edSensitivity + 2))
		{
			// can take 9-12 sf
			//			  std::cout<< "Sensitivity:" <<*(edSensitivity+2) << " push_back 3" <<std::endl;
			availableSFsVector.push_back(3);
		}
		else if (highestRxPower > *(edSensitivity + 3))
		{
			// can take 10-12 sf
			//			  std::cout<< "Sensitivity:" << *(edSensitivity+3) << " push_back 2" <<std::endl;
			availableSFsVector.push_back(2);
		}
		else if (highestRxPower > *(edSensitivity + 4))
		{
			// can take 11 or 12 sf
			//			  std::cout<< "Sensitivity:" << *(edSensitivity+4) << " push_back 1" <<std::endl;
			availableSFsVector.push_back(1);
		}
		else if (highestRxPower > *(edSensitivity + 5))
		{
			// can only take 12 sf
			//			  std::cout<< "Sensitivity:" << *(edSensitivity+5) << " push_back 0" <<std::endl;
			availableSFsVector.push_back(0);
		}
		else // Device is out of range. Assign SF12.
		{
			// can only take 12 sf
			//			  std::cout<< "Sensitivity:" << *(edSensitivity+5) << " push_back 0 (out of range)" <<std::endl;
			availableSFsVector.push_back(0);
		}
	} //  end first for loop

	int elementIndex = 0;
	//cycle over all end nodes again

	//sort distanceVector
	distanceVectorSorted=distanceVector;
	std::sort(distanceVectorSorted.begin(),distanceVectorSorted.end());

	//print all four vectors here for debug
	/*	for (size_t i=0;i!=availableSFsVector.size();i++){
		std::cout << availableSFsVector[i] <<std::endl;
	}
	for (size_t i=0;i!=distanceVector.size();i++){
		std::cout << distanceVector[i] <<std::endl;
	}
	for (size_t i=0;i!=distanceVectorSorted.size();i++){
		//sorted
		std::cout << "Sorted: " << distanceVectorSorted[i] <<std::endl;
	}
	for (size_t i=0;i!=SFtoAssignVector.size();i++){
		std::cout << SFtoAssignVector[i] <<std::endl;
	}
	 */

	for (size_t i=0;i!=distanceVectorSorted.size();i++){

		// Find current element back into original distanceVector
		std::vector<double>::iterator it = std::find(distanceVector.begin(), distanceVector.end(), distanceVectorSorted[i]);
		elementIndex = std::distance(distanceVector.begin(), it);

		//which SF can be assigned to this node?
		int SFtoAssignToNode = 0;
		int possibleSF = availableSFsVector[elementIndex];

		if (possibleSF == 5)
		{
			//assign lowest possible one that is available
			for (size_t i=0;i!=SFtoAssignVector.size();i++){
				if (SFtoAssignVector[i] != 0){
					SFtoAssignToNode = i;
					break;
				}
				// if all were taken
				SFtoAssignToNode = 5;
				//std::cout << "not breaking, assigning sf 12 despite possible SF: " << possibleSF << std::endl;
			}
		} else if (possibleSF == 4) {
			//assign lowest possible greater than 7 that is available
			for (size_t i=1;i!=SFtoAssignVector.size();i++){
				if (SFtoAssignVector[i] != 0){
					SFtoAssignToNode = i;
					break;
				}
				// if all were taken
				SFtoAssignToNode = 5;
				//std::cout << "not breaking, assigning sf 12 despite possible SF: " << possibleSF << std::endl;
			}

		} else if (possibleSF == 3) {
			//assign lowest possible greater than 8 that is available
			for (size_t i=2;i!=SFtoAssignVector.size();i++){
				if (SFtoAssignVector[i] != 0){
					SFtoAssignToNode = i;
					break;
				}
				// if all were taken
				SFtoAssignToNode = 5;
				//std::cout << "not breaking, assigning sf 12 despite possible SF: " << possibleSF << std::endl;
			}

		} else if (possibleSF == 2) {
			//assign lowest possible greater than 9 that is available
			for (size_t i=3;i!=SFtoAssignVector.size();i++){
				if (SFtoAssignVector[i] != 0){
					SFtoAssignToNode = i;
					break;
				}
				// if all were taken
				SFtoAssignToNode = 5;
				//std::cout << "not breaking, assigning sf 12 despite possible SF: " << possibleSF << std::endl;
			}

		} else if (possibleSF == 1) {
			//assign lowest possible greater than 10 that is available
			for (size_t i=4;i!=SFtoAssignVector.size();i++){
				if (SFtoAssignVector[i] != 0){
					SFtoAssignToNode = i;
					break;
				}
				// if all were taken
				SFtoAssignToNode = 5;
				//std::cout << "not breaking, assigning sf 12 despite possible SF: " << possibleSF << std::endl;
			}

		} else if (possibleSF == 0) {
			//assign 12
			SFtoAssignToNode = 5;
		}

		// reduce the number of possible node with the SF that was just assigned by 1
		SFtoAssignVector[SFtoAssignToNode] = SFtoAssignVector[SFtoAssignToNode] -1;


		// assign SF to node at position elementIndex
		Ptr<Node> object = endDevices.Get(elementIndex);
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
		NS_ASSERT (mac != 0);

		int adjSF = 7+SFtoAssignToNode;
		//		std::cout<< "Element Index: " << elementIndex << ", possible SF: " << possibleSF << ", SFtoAssignToNode: " << SFtoAssignToNode << ", adjSF: " << adjSF << std::endl;
		// set to SF7
		mac ->SetDataRate(12-adjSF);
	}

	return 1;
}



int
LorawanMacHelper::SetSpreadingFactorsUpBasedOnDistanceAlone (NodeContainer endDevices, NodeContainer gateways,
		Ptr<LoraChannel> channel)
{
	NS_LOG_FUNCTION_NOARGS ();

	std::vector<double> distanceVector;
	double distanceNodeToGW = 0;
	std::vector<double> distanceVectorSorted;

	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		//		    std::cout<<"Inside first loop"<<std::endl;
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();

		int elementIndex = 0;
		std::vector<double> relativeDistances;

		//std::cout << "Before loop" <<std::endl;
		for (NodeContainer::Iterator i = gateways.Begin (); i != gateways.End (); ++i) {
			//std::cout << "loop starts" <<std::endl;
			// computing the distance from each gateway to find the best/worst one
			Ptr<Node> currGateway = *i;
			Ptr<MobilityModel> currGatewayPosition = currGateway->GetObject<MobilityModel> ();

			relativeDistances.push_back(position->GetDistanceFrom (currGatewayPosition));

			// print distances
			for (size_t i=0;i!=relativeDistances.size();i++){
				//std::cout << "Relative Distances: " << relativeDistances[i] <<std::endl;
			}

			elementIndex = std::min_element(relativeDistances.begin(),relativeDistances.end()) - relativeDistances.begin();
			//std::cout << "Index: " << elementIndex <<std::endl;
		}

		//Ptr<Node> bestGateway = gateways.Get(elementIndex);
		Ptr<MobilityModel> bestGatewayPosition = gateways.Get(elementIndex)->GetObject<MobilityModel> ();
		distanceNodeToGW= position->GetDistanceFrom (bestGatewayPosition);
		std::vector<int> Sfboundaries{ 3259, 4209, 5436,7021,8690,10755 };

		if(distanceNodeToGW <= Sfboundaries[0])
		{
			mac->SetDataRate (5);
		}
		else if (distanceNodeToGW <= Sfboundaries[1] && distanceNodeToGW > Sfboundaries[0])
		{
			mac->SetDataRate (4);
		}
		else if (distanceNodeToGW <= Sfboundaries[2] && distanceNodeToGW > Sfboundaries[1])
		{
			mac->SetDataRate (3);
		}
		else if (distanceNodeToGW <= Sfboundaries[3] && distanceNodeToGW > Sfboundaries[2])
		{
			mac->SetDataRate (2);
		}
		else if (distanceNodeToGW <= Sfboundaries[4] && distanceNodeToGW > Sfboundaries[3])
		{
			mac->SetDataRate (1);
		}
		else if (distanceNodeToGW <= Sfboundaries[5] && distanceNodeToGW > Sfboundaries[4])
		{
			mac->SetDataRate (0);
		}
		else // Device is out of range. Assign SF12.
		{
			mac->SetDataRate (0);
		}


	} // end loop on nodes

	return 1;
}
// Function added by BC
int
LorawanMacHelper::SetDistributedSpreadingFactorsAtRandom (NodeContainer endDevices, NodeContainer gateways, Ptr<LoraChannel> channel, std::vector<int> SFdistribution)
{
	//std::cout<<"Start"<<std::endl;
	std::vector<double> distanceVector;
	std::vector<double> distanceVectorSorted;
	std::vector<int> availableSFsVector;
	std::vector<int> SFtoAssignVector = SFdistribution;

	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		//    std::cout<<"Inside first loop"<<std::endl;
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();

		int elementIndex = 0;
		//
		for (NodeContainer::Iterator i = gateways.Begin (); i != gateways.End (); ++i) {
			// computing the distance from each gateway to find the best/worst one
			Ptr<Node> currGateway = *i;
			Ptr<MobilityModel> currGatewayPosition = currGateway->GetObject<MobilityModel> ();

			std::vector<double> relativeDistances;
			relativeDistances.push_back(position->GetDistanceFrom (currGatewayPosition));
			//ElementIndex = std::max_element(relativeDistances.begin(),relativeDistances.end()) - relativeDistances.begin();
			elementIndex = std::min_element(relativeDistances.begin(),relativeDistances.end()) - relativeDistances.begin();
		}

		//Ptr<Node> bestGateway = gateways.get(elementIndex);
		Ptr<MobilityModel> bestGatewayPosition = gateways.Get(elementIndex)->GetObject<MobilityModel> ();
		distanceVector.push_back(position->GetDistanceFrom (bestGatewayPosition));
		// Assume devices transmit at 14 dBm
		double highestRxPower = channel->GetRxPower (14, position, bestGatewayPosition);

		// Get sensitivity values
		Ptr<EndDeviceLoraPhy> edPhy = loraNetDevice->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
		const double *edSensitivity = edPhy->sensitivity;

		//	std::cout<< "Distance:" << position->GetDistanceFrom (bestGatewayPosition) << ", RxPower: " << highestRxPower <<std::endl;
		if (highestRxPower > *edSensitivity)
		{
			// can take any sf
			//		  std::cout<< "Sensitivity:" << *edSensitivity << " push_back 5" <<std::endl;
			availableSFsVector.push_back(5);
		}
		else if (highestRxPower > *(edSensitivity + 1))
		{
			// can take 8-12 sf
			//	  std::cout<< "Sensitivity:" << *(edSensitivity +1)<< " push_back 4" <<std::endl;
			availableSFsVector.push_back(4);
		}
		else if (highestRxPower > *(edSensitivity + 2))
		{
			// can take 9-12 sf
			//	  std::cout<< "Sensitivity:" <<*(edSensitivity+2) << " push_back 3" <<std::endl;
			availableSFsVector.push_back(3);
		}
		else if (highestRxPower > *(edSensitivity + 3))
		{
			// can take 10-12 sf
			//		  std::cout<< "Sensitivity:" << *(edSensitivity+3) << " push_back 2" <<std::endl;
			availableSFsVector.push_back(2);
		}
		else if (highestRxPower > *(edSensitivity + 4))
		{
			// can take 11 or 12 sf
			//		  std::cout<< "Sensitivity:" << *(edSensitivity+4) << " push_back 1" <<std::endl;
			availableSFsVector.push_back(1);
		}
		else if (highestRxPower > *(edSensitivity + 5))
		{
			// can only take 12 sf
			//			  std::cout<< "Sensitivity:" << *(edSensitivity+5) << " push_back 0" <<std::endl;
			availableSFsVector.push_back(0);
		}
		else // Device is out of range. Assign SF12.
		{
			// can only take 12 sf
			//		  std::cout<< "Sensitivity:" << *(edSensitivity+5) << " push_back 0 (out of range)" <<std::endl;
			availableSFsVector.push_back(0);
		}
	} //  end first for loop

	int elementIndex = 0;
	//cycle over all end nodes again

	//sort distanceVector
	distanceVectorSorted=distanceVector;
	std::sort(distanceVectorSorted.begin(),distanceVectorSorted.end());

	/*
	for (size_t i=0;i!=distanceVectorSorted.size();i++){
		//sorted
		std::cout << "Sorted: " << distanceVectorSorted[i] <<std::endl;
	}
	 */
	for (size_t i=0;i!=distanceVectorSorted.size();i++){
		//	  std::cout<<"Inside second loop"<<std::endl;


		/*			//print all three vectors here for debug
			for (size_t i=0;i!=availableSFsVector.size();i++){
				std::cout << availableSFsVector[i] <<std::endl;
			}
			for (size_t i=0;i!=distanceVector.size();i++){
				std::cout << distanceVector[i] <<std::endl;
			}
			for (size_t i=0;i!=distanceVectorSorted.size();i++){
				//sorted
				std::cout << "Sorted: " << distanceVectorSorted[i] <<std::endl;
			}
			for (size_t i=0;i!=SFtoAssignVector.size();i++){
				std::cout << SFtoAssignVector[i] <<std::endl;
			}
		 */

		// Find current element back into original distanceVector
		std::vector<double>::iterator it = std::find(distanceVector.begin(), distanceVector.end(), distanceVectorSorted[i]);
		elementIndex = std::distance(distanceVector.begin(), it);

		//which SF can be assigned to this node?
		int SFtoAssignToNode = 0;
		int possibleSF = availableSFsVector[elementIndex];
		int randomSF;

		if (possibleSF == 5)
		{			  //assign random between 7-12 possible one that is available
			while(1){
				randomSF=rand()%6+7;
				//				  std::cout<<"Inside possibleSF " << possibleSF << ", randomSF: " << randomSF << std::endl;
				if (SFtoAssignVector[randomSF-7] != 0){
					SFtoAssignToNode = randomSF-7;
					//					  std::cout<<"Assigned " << SFtoAssignToNode <<std::endl;
					break;
				}

				if ((SFtoAssignVector[0] <= 0) && (SFtoAssignVector[1]<= 0) && (SFtoAssignVector[2]<= 0) && (SFtoAssignVector[3]<= 0) && (SFtoAssignVector[4]<= 0) && (SFtoAssignVector[5] <= 0)){
					//assign sf 12
					//				  std::cout<<"All were taken, assign 12" << std::endl;
					SFtoAssignToNode = 5;
					break;
				}
			}
		} else if (possibleSF == 4) {
			//assign random between 8-12 that is available
			while(1){
				randomSF=rand()%5+8;
				//		  std::cout<<"Inside possibleSF " << possibleSF << ", randomSF: " << randomSF <<std::endl;
				if (SFtoAssignVector[randomSF-7] != 0){
					SFtoAssignToNode = randomSF-7;
					//				  std::cout<<"Assigned " << SFtoAssignToNode <<std::endl;
					break;
				}
				if ((SFtoAssignVector[1]<= 0) && (SFtoAssignVector[2]<= 0) && (SFtoAssignVector[3]<= 0) && (SFtoAssignVector[4]<= 0) && (SFtoAssignVector[5] <= 0)){
					//assign sf 12
					//					  std::cout<<"All were taken, assign 12" << std::endl;
					SFtoAssignToNode = 5;
					break;
				}
			}
		} else if (possibleSF == 3) {
			//assign lowest possible greater than 8 that is available
			while(1){
				randomSF=rand()%4+9;
				//				  std::cout<<"Inside possibleSF " << possibleSF << ", randomSF: " << randomSF <<std::endl;
				if (SFtoAssignVector[randomSF-7] != 0){
					SFtoAssignToNode = randomSF-7;
					//					  std::cout<<"Assigned " << SFtoAssignToNode <<std::endl;
					break;
				}
				if ( (SFtoAssignVector[2]<= 0) && (SFtoAssignVector[3]<= 0) && (SFtoAssignVector[4]<= 0) && (SFtoAssignVector[5] <= 0)){
					//assign sf 12
					//					  std::cout<<"All were taken, assign 12" << std::endl;
					SFtoAssignToNode = 5;
					break;
				}
			}
		} else if (possibleSF == 2) {
			//assign lowest possible greater than 9 that is available
			while(1){
				randomSF=rand()%3+10;
				//				  std::cout<<"Inside possibleSF " << possibleSF << ", randomSF: " << randomSF <<std::endl;
				if (SFtoAssignVector[randomSF-7] != 0){
					SFtoAssignToNode = randomSF-7;
					//					  std::cout<<"Assigned " << SFtoAssignToNode <<std::endl;
					break;
				}
				if ( (SFtoAssignVector[3]<= 0) && (SFtoAssignVector[4]<= 0) && (SFtoAssignVector[5] <= 0)){
					//assign sf 12
					//					  std::cout<<"All were taken, assign 12" << std::endl;
					SFtoAssignToNode = 5;
					break;
				}
			}
		} else if (possibleSF == 1) {
			//assign lowest possible greater than 10 that is available
			while(1){
				randomSF=rand()%2+11;
				//				  std::cout<<"Inside possibleSF " << possibleSF << ", randomSF: " << randomSF <<std::endl;
				if (SFtoAssignVector[randomSF-7] != 0){
					SFtoAssignToNode = randomSF-7;
					//					  std::cout<<"Assigned " << SFtoAssignToNode <<std::endl;
					break;
				}
				if ( (SFtoAssignVector[4]<= 0) && (SFtoAssignVector[5] <= 0)){
					//assign sf 12
					//					  std::cout<<"All were taken, assign 12" << std::endl;
					SFtoAssignToNode = 5;
					break;
				}
			}
		} else if (possibleSF == 0) {
			//assign 12
			//			  std::cout<<"Inside possibleSF " << possibleSF << std::endl;
			SFtoAssignToNode = 5;
		}

		// reduce the number of possible node with the SF that was just assigned by 1
		SFtoAssignVector[SFtoAssignToNode] = SFtoAssignVector[SFtoAssignToNode] -1;


		// assign SF to node at position elementIndex
		Ptr<Node> object = endDevices.Get(elementIndex);
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
		NS_ASSERT (mac != 0);

		int adjSF = 7+SFtoAssignToNode;
		//		std::cout<< "Element Index: " << elementIndex << ", possible SF: " << possibleSF << ", SFtoAssignToNode: " << SFtoAssignToNode << ", adjSF: " << adjSF << std::endl;
		// set to SF7
		mac ->SetDataRate(12-adjSF);
		//        int sf = mac -> GetDataRate();
		//        std::cout << "sf is, within lorawanMacHelper: " << sf << std::endl;

		/*
		//print all three vectors here for debug
		for (size_t i=0;i!=availableSFsVector.size();i++){
			std::cout << availableSFsVector[i] <<std::endl;
		}
		for (size_t i=0;i!=distanceVector.size();i++){
			std::cout << distanceVector[i] <<std::endl;
		}
		for (size_t i=0;i!=SFtoAssignVector.size();i++){
			std::cout << SFtoAssignVector[i] <<std::endl;
		}*/
	}

	return 1;
}

// Function Added by BC
std::vector<int>LorawanMacHelper::GetSFDistribution (NodeContainer endDevices)
{

	std::vector<int> sfQuantity (7, 0);

	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<EndDeviceLorawanMac> mac2 = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();
		NS_ASSERT (mac2 != 0);
		switch (mac2->GetDataRate()) {
		case 5:
			sfQuantity[0] = sfQuantity[0] + 1;
			break;
		case 4:
			sfQuantity[1] = sfQuantity[1] + 1;
			break;
		case 3:
			sfQuantity[2] = sfQuantity[2] + 1;
			break;
		case 2:
			sfQuantity[3] = sfQuantity[3] + 1;
			break;
		case 1:
			sfQuantity[4] = sfQuantity[4] + 1;
			break;
		case 0:
			sfQuantity[5] = sfQuantity[5] + 1;
			break;
		}


	}
	return sfQuantity;
}

//Added by BC
void LorawanMacHelper::PrintSFDistribution (NodeContainer endDevices)
{

	std::vector<int> sfQuantity (7, 0);

	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<EndDeviceLorawanMac> mac2 = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();
		NS_ASSERT (mac2 != 0);
		switch (mac2->GetDataRate()) {
		case 5:
			sfQuantity[0] = sfQuantity[0] + 1;
			break;
		case 4:
			sfQuantity[1] = sfQuantity[1] + 1;
			break;
		case 3:
			sfQuantity[2] = sfQuantity[2] + 1;
			break;
		case 2:
			sfQuantity[3] = sfQuantity[3] + 1;
			break;
		case 1:
			sfQuantity[4] = sfQuantity[4] + 1;
			break;
		case 0:
			sfQuantity[5] = sfQuantity[5] + 1;
			break;
		}


	}

	std::cout << std::endl;
	// Print out SF distribution
	for(size_t i=0; i< sfQuantity.size(); i++) {
		std::cout << "Nodes with SF" << i+7 << ": " << sfQuantity[i] << std::endl;
	}

	std::cout << std::endl;
}

//Added by BC
void LorawanMacHelper::PrintSFDistributionToFile (NodeContainer endDevices, std:: string filename)
{
	std::ofstream output;
	output.open (filename, std::ofstream::out | std::ofstream::app);

	std::vector<int> sfQuantity (7, 0);

	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice(0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<EndDeviceLorawanMac> mac2 = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();
		NS_ASSERT (mac2 != 0);
		switch (mac2->GetDataRate()) {
		case 5:
			sfQuantity[0] = sfQuantity[0] + 1;
			break;
		case 4:
			sfQuantity[1] = sfQuantity[1] + 1;
			break;
		case 3:
			sfQuantity[2] = sfQuantity[2] + 1;
			break;
		case 2:
			sfQuantity[3] = sfQuantity[3] + 1;
			break;
		case 1:
			sfQuantity[4] = sfQuantity[4] + 1;
			break;
		case 0:
			sfQuantity[5] = sfQuantity[5] + 1;
			break;
		}


	}

	output << std::endl;
	// Print out SF distribution
	for(size_t i=0; i< sfQuantity.size(); i++) {
		output << "Nodes with assigned SF" << i+7 << ": " << sfQuantity[i] << std::endl;
	}

	std::cout << std::endl;
	output.close();
}

std::vector<int>
LorawanMacHelper::SetSpreadingFactorsGivenDistribution (NodeContainer endDevices,
		NodeContainer gateways,
		std::vector<double> distribution)
{
	NS_LOG_FUNCTION_NOARGS ();

	std::vector<int> sfQuantity (7, 0);
	Ptr<UniformRandomVariable> uniformRV = CreateObject<UniformRandomVariable> ();
	std::vector<double> cumdistr (6);
	cumdistr[0] = distribution[0];
	for (int i = 1; i < 7; ++i)
	{
		cumdistr[i] = distribution[i] + cumdistr[i - 1];
	}

	NS_LOG_DEBUG ("Distribution: " << distribution[0] << " " << distribution[1] << " "
			<< distribution[2] << " " << distribution[3] << " "
			<< distribution[4] << " " << distribution[5]);
	NS_LOG_DEBUG ("Cumulative distribution: " << cumdistr[0] << " " << cumdistr[1] << " "
			<< cumdistr[2] << " " << cumdistr[3] << " "
			<< cumdistr[4] << " " << cumdistr[5]);

	for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
	{
		Ptr<Node> object = *j;
		Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
		NS_ASSERT (position != 0);
		Ptr<NetDevice> netDevice = object->GetDevice (0);
		Ptr<LoraNetDevice> loraNetDevice = netDevice->GetObject<LoraNetDevice> ();
		NS_ASSERT (loraNetDevice != 0);
		Ptr<ClassAEndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<ClassAEndDeviceLorawanMac> ();
		NS_ASSERT (mac != 0);

		double prob = uniformRV->GetValue (0, 1);

		NS_LOG_DEBUG ("Probability: " << prob);
		if (prob < cumdistr[0])
		{
			NS_LOG_DEBUG ("SF7");
			mac->SetDataRate (5);
			sfQuantity[0] = sfQuantity[0] + 1;
		}
		else if (prob > cumdistr[0] && prob < cumdistr[1])
		{
			NS_LOG_DEBUG ("SF8");
			mac->SetDataRate (4);
			sfQuantity[1] = sfQuantity[1] + 1;
		}
		else if (prob > cumdistr[1] && prob < cumdistr[2])
		{
			mac->SetDataRate (3);
			sfQuantity[2] = sfQuantity[2] + 1;
		}
		else if (prob > cumdistr[2] && prob < cumdistr[3])
		{
			mac->SetDataRate (2);
			sfQuantity[3] = sfQuantity[3] + 1;
		}
		else if (prob > cumdistr[3] && prob < cumdistr[4])
		{
			mac->SetDataRate (1);
			sfQuantity[4] = sfQuantity[4] + 1;
		}
		else
		{
			mac->SetDataRate (0);
			sfQuantity[5] = sfQuantity[5] + 1;
		}

	} // end loop on nodes

	return sfQuantity;

} //  end function

} // namespace lorawan
} // namespace ns3
