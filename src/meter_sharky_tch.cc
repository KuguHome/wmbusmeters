/*
 Copyright (C) 2021 Vincent Privat

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"meters.h"
#include"meters_common_implementation.h"
#include"dvparser.h"
#include"wmbus.h"
#include"wmbus_utils.h"
#include"manufacturer_specificities.h"

struct MeterSharkyTch : public virtual HeatMeter, public virtual MeterCommonImplementation {
    MeterSharkyTch(MeterInfo &mi);

    double totalEnergyConsumption(Unit u);
    double totalVolume(Unit u);
    double volumeFlow(Unit u);
    double power(Unit u);
    double flowTemperature(Unit u);
    double returnTemperature(Unit u);
    double totalEnergyConsumptionTariff1(Unit u);
    double operatingTime(Unit u);

private:
    void processContent(Telegram *t);

    double total_energy_kwh_ {};
    double total_volume_m3_ {};
    double volume_flow_m3h_ {};
    double power_w_ {};
    double flow_temperature_c_ {};
    double return_temperature_c_ {};
    double total_energy_tariff1_kwh_ {};
    double operating_time_t_ {};
    vector<uint32_t> keys;
};

MeterSharkyTch::MeterSharkyTch(MeterInfo &mi) :
    MeterCommonImplementation(mi, MeterType::SHARKYTCH)
{
    addLinkMode(LinkMode::T1);

    addPrint("total_energy_consumption", Quantity::Energy,
             [&](Unit u){ return totalEnergyConsumption(u); },
             "The total energy consumption recorded by this meter.",
             true, true);

    addPrint("total_volume", Quantity::Volume,
             [&](Unit u){ return totalVolume(u); },
             "The total volume recorded by this meter.",
             true, true);

    addPrint("volume_flow", Quantity::Flow,
             [&](Unit u){ return volumeFlow(u); },
             "The current flow.",
             true, true);

    addPrint("power", Quantity::Power,
             [&](Unit u){ return power(u); },
             "The power.",
             true, true);

    addPrint("flow_temperature", Quantity::Temperature,
             [&](Unit u){ return flowTemperature(u); },
             "The flow temperature.",
             true, true);

    addPrint("return_temperature", Quantity::Temperature,
             [&](Unit u){ return returnTemperature(u); },
             "The return temperature.",
             true, true);

    addPrint("total_energy_consumption_tariff1", Quantity::Energy,
             [&](Unit u){ return totalEnergyConsumptionTariff1(u); },
             "The total energy consumption recorded by this meter on tariff 1.",
             true, true);

    addPrint("operating_time", Quantity::Time,
             [&](Unit u){ return operatingTime(u); },
             "The temperature difference.",
             true, true);
}

shared_ptr<HeatMeter> createSharkyTch(MeterInfo &mi) {
    return shared_ptr<HeatMeter>(new MeterSharkyTch(mi));
}

double MeterSharkyTch::totalEnergyConsumption(Unit u)
{
    assertQuantity(u, Quantity::Energy);
    return convert(total_energy_kwh_, Unit::KWH, u);
}

double MeterSharkyTch::totalVolume(Unit u)
{
    assertQuantity(u, Quantity::Volume);
    return convert(total_volume_m3_, Unit::M3, u);
}

double MeterSharkyTch::volumeFlow(Unit u)
{
    assertQuantity(u, Quantity::Flow);
    return convert(volume_flow_m3h_, Unit::M3H, u);
}

double MeterSharkyTch::power(Unit u)
{
    assertQuantity(u, Quantity::Power);
    return convert(power_w_, Unit::KW, u);
}

double MeterSharkyTch::flowTemperature(Unit u)
{
    assertQuantity(u, Quantity::Temperature);
    return convert(flow_temperature_c_, Unit::C, u);
}

double MeterSharkyTch::returnTemperature(Unit u)
{
    assertQuantity(u, Quantity::Temperature);
    return convert(return_temperature_c_, Unit::C, u);
}

double MeterSharkyTch::totalEnergyConsumptionTariff1(Unit u)
{
    assertQuantity(u, Quantity::Energy);
    return convert(total_energy_tariff1_kwh_, Unit::KWH, u);
}

double MeterSharkyTch::operatingTime(Unit u)
{
    assertQuantity(u, Quantity::Time);
    return convert(operating_time_t_, Unit::Second, u);
}

void MeterSharkyTch::processContent(Telegram *t)
{
    /*
      (wmbus) 0f: 0C dif (8 digit BCD Instantaneous value)
      (wmbus) 10: 06 vif (Energy kWh)
      (wmbus) 11: 22052400
      (wmbus) 15: 0C dif (8 digit BCD Instantaneous value)
      (wmbus) 16: 13 vif (Volume m^3)
      (wmbus) 17: 9654256
      (wmbus) 18: 0B dif (6 digit BCD Instantaneous value)
      (wmbus) 1c: 3B vif (Volume flow m^3/h)
      (wmbus) 1d: 928
      (wmbus) 1e: 0C dif (8 digit BCD Instantaneous value)
      (wmbus) 22: 2B vif (Power W)
      (wmbus) 23: 26470
      (wmbus) 24: 0A dif (4 digit BCD Instantaneous value)
      (wmbus) 25: 5A vif (Flow temperature 10⁻¹ °C)
      (wmbus) 29: 934
      (wmbus) 2a: 0A dif (4 digit BCD Instantaneous value)
      (wmbus) 2b: 5E vif (Return temperature 10⁻¹ °C)
      (wmbus) 2c: 684
      (wmbus) 30: 8C dif (8 digit BCD Instantaneous value)
      (wmbus) 31: 10 dife (subunit=0 tariff=1 storagenr=0)
      (wmbus) 32: 06 vif (Energy kWh)
      (wmbus) 33: 00000000
      (wmbus) 34: 0A dif (4 digit BCD Instantaneous value)
      (wmbus) 38: A6 dif (??)
      (wmbus) 39: 18 vif (Operating time seconds)
      (wmbus) 3a: 0000
    */

    int offset;
    string key;

    if (findKey(MeasurementType::Instantaneous, ValueInformation::EnergyWh, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &total_energy_kwh_);
        t->addMoreExplanation(offset, " total energy consumption (%f kWh)", total_energy_kwh_);
    }
    if (findKey(MeasurementType::Instantaneous, ValueInformation::Volume, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &total_volume_m3_);
        t->addMoreExplanation(offset, " total volume (%f ㎥)", total_volume_m3_);
    }

    if (findKey(MeasurementType::Instantaneous, ValueInformation::VolumeFlow, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &volume_flow_m3h_);
        t->addMoreExplanation(offset, " volume flow (%f ㎥/h)", volume_flow_m3h_);
    }

    if (findKey(MeasurementType::Instantaneous, ValueInformation::PowerW, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &power_w_);
        t->addMoreExplanation(offset, " power (%f W)", power_w_);
    }

    if (findKey(MeasurementType::Instantaneous, ValueInformation::FlowTemperature, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &flow_temperature_c_);
        t->addMoreExplanation(offset, " flow temperature (%f °C)", flow_temperature_c_);
    }

    if (findKey(MeasurementType::Instantaneous, ValueInformation::ReturnTemperature, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &return_temperature_c_);
        t->addMoreExplanation(offset, " return temperature (%f °C)", return_temperature_c_);
    }

    if (findKey(MeasurementType::Instantaneous, ValueInformation::EnergyWh, 0, 1, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &total_energy_tariff1_kwh_);
        t->addMoreExplanation(offset, " total energy tariff 1 (%f kwh)", total_energy_tariff1_kwh_);
    }


    if (findKey(MeasurementType::Instantaneous, ValueInformation::OperatingTime, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &operating_time_t_);
        t->addMoreExplanation(offset, " operating time (%f seconds)", operating_time_t_);
    }
}
