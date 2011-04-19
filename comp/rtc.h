/* RTC:Joy
 *
 * Copyright (C) 2011
 *     Geoffrey Biggs
 *     RT-Synthesis Research Group
 *     Intelligent Systems Research Institute,
 *     National Institute of Advanced Industrial Science and Technology (AIST),
 *     Japan
 *     All rights reserved.
 * Licensed under the Eclipse Public License -v 1.0 (EPL)
 * http://www.opensource.org/licenses/eclipse-1.0.txt
 *
 * Component header file.
 */


#ifndef RTCJOY_H__
#define RTCJOY_H__

#include <rtm/Manager.h>
#include <rtm/DataFlowComponentBase.h>
#include <rtm/OutPort.h>
#include <rtm/idl/ExtendedDataTypes.hh>

#include <idl/rtcjoy_types.hh>


class RTCJoy
: public RTC::DataFlowComponentBase
{
    public:
        RTCJoy(RTC::Manager* manager);
        ~RTCJoy();

        virtual RTC::ReturnCode_t onInitialize();
        virtual RTC::ReturnCode_t onFinalize();
        //virtual RTC::ReturnCode_t onActivated(RTC::UniqueId ec_id);
        //virtual RTC::ReturnCode_t onDeactivated(RTC::UniqueId ec_id);
        virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

    private:
        RTCJoyTypes::JSAxis axes_;
        RTC::OutPort<RTCJoyTypes::JSAxis> axes_port_;
        RTCJoyTypes::JSBtn buttons_;
        RTC::OutPort<RTCJoyTypes::JSBtn> buttons_port_;
        RTC::TimedPoint2D xy_;
        RTC::OutPort<RTC::TimedPoint2D> xy_port_;
        RTC::TimedVelocity2D va_;
        RTC::OutPort<RTC::TimedVelocity2D> va_port_;

        std::string device_;
        int x_axis_;
        int y_axis_;
        float scale_v_, scale_a_;

        int js_; // Joystick device file descriptor
};


extern "C"
{
    DLL_EXPORT void rtc_init(RTC::Manager* manager);
};

#endif // RTCJOY_H__

