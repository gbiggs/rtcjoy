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
 * Component source file.
 */


#include "rtc.h"

#include <cstring>
#include <cerrno>
#include <iostream>
#include <linux/joystick.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


RTCJoy::RTCJoy(RTC::Manager* manager)
    : RTC::DataFlowComponentBase(manager),
    axes_port_("axes", axes_), buttons_port_("buttons", buttons_),
    xy_port_("xy", xy_), va_port_("va", va_),
    device_("/dev/js0"), x_axis_(0), y_axis_(1), scale_v_(-1.0), scale_a_(1.0),
    js_(-1)
{
}


RTCJoy::~RTCJoy()
{
}


RTC::ReturnCode_t RTCJoy::onInitialize()
{
    bindParameter("device", device_, "/dev/input/js0");
    bindParameter("x_axis", x_axis_, "0");
    bindParameter("y_axis", y_axis_, "1");
    bindParameter("scale_v", scale_v_, "-1.0");
    bindParameter("scale_a", scale_a_, "1.0");
    std::string active_set =
        m_properties.getProperty("configuration.active_config", "default");
    m_configsets.update(active_set.c_str());

    addOutPort(axes_port_.getName(), axes_port_);
    addOutPort(buttons_port_.getName(), buttons_port_);
    addOutPort(xy_port_.getName(), xy_port_);
    addOutPort(va_port_.getName(), va_port_);
#if defined(RTDOC_SUPPORT)
    /*axes_port_.addProperty("description",
            "Axis values, normalised to between -1.0 and 1.0.");
    buttons_port_.addProperty("description",
            "Button press and release events.");
    xy_port_.addProperty("description", "X/Y position.");
    va_port_.addProperty("description", "Forward and angular velocity based on xy.");*/
#endif //defined(RTDOC_SUPPORT)

    // Open the joystick device
    if ((js_ = open(device_.c_str(), O_RDONLY)) == -1)
    {
        std::cerr << "RTC:Joy: Error opening joystick device: " <<
            strerror(errno) << '\n';
        return RTC::RTC_ERROR;
    }
    int num_axes(0), num_buttons(0);
    ioctl(js_, JSIOCGAXES, &num_axes);
    ioctl(js_, JSIOCGBUTTONS, &num_buttons);
    char name_str[80];
    ioctl(js_, JSIOCGNAME(80), &name_str);
    if (fcntl(js_, F_SETFL, O_NONBLOCK) < 0)
    {
        std::cerr <<
            "RTC:Joy: Error setting joystick device to non-blocking: " <<
            strerror(errno) << '\n';
        return RTC::RTC_ERROR;
    }
    std::cout << "RTC:Joy: Opened joystick " << name_str << ", " <<
        num_axes << " axes, " << num_buttons << " buttons.\n";

    xy_.data.x = 0.0;
    xy_.data.y = 0.0;
    va_.data.vx = 0.0;
    va_.data.vy = 0.0;
    va_.data.va = 0.0;

    return RTC::RTC_OK;
}


RTC::ReturnCode_t RTCJoy::onFinalize()
{
    // Close the joystick device.
    if (js_ != -1)
    {
        close(js_);
    }

    return RTC::RTC_OK;
}


/*RTC::ReturnCode_t RTCJoy::onActivated(RTC::UniqueId ec_id)
{
    return RTC::RTC_OK;
}*/


/*RTC::ReturnCode_t RTCJoy::onDeactivated(RTC::UniqueId ec_id)
{
    return RTC::RTC_OK;
}*/


RTC::ReturnCode_t RTCJoy::onExecute(RTC::UniqueId ec_id)
{
    struct js_event ev;

    if(read(js_, &ev, sizeof(ev)) < 0)
    {
        if (errno == EAGAIN)
        {
            // No data ready
            return RTC::RTC_OK;
        }
        std::cerr << "RTC:Joy: Error reading joystick device: " <<
            strerror(errno) << '\n';
        return RTC::RTC_ERROR;
    }

    if (ev.type & JS_EVENT_INIT)
    {
        // Ignore init events
        return RTC::RTC_OK;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    switch (ev.type)
    {
        case JS_EVENT_AXIS:
            axes_.axis = int(ev.number);
            // Maximum values of Linux joystick axis are +/- 32767
            axes_.value = double(ev.value) / 32767.0;
            axes_.tm.sec = ts.tv_sec;
            axes_.tm.nsec = ts.tv_nsec;
            axes_port_.write();
            if (ev.number == x_axis_)
            {
                xy_.data.x = axes_.value;
                xy_.tm.sec = ts.tv_sec;
                xy_.tm.nsec = ts.tv_nsec;
                xy_port_.write();
                va_.data.va = axes_.value * scale_a_;
                va_.tm.sec = ts.tv_sec;
                va_.tm.nsec = ts.tv_nsec;
                va_port_.write();
            }
            else if (ev.number == y_axis_)
            {
                xy_.data.y = axes_.value;
                xy_.tm.sec = ts.tv_sec;
                xy_.tm.nsec = ts.tv_nsec;
                xy_port_.write();
                va_.data.vx = axes_.value * scale_v_;
                va_.tm.sec = ts.tv_sec;
                va_.tm.nsec = ts.tv_nsec;
                va_port_.write();
            }
            break;
        case JS_EVENT_BUTTON:
            buttons_.button = int(ev.number);
            buttons_.pressed = bool(ev.value);
            buttons_.tm.sec = ts.tv_sec;
            buttons_.tm.nsec = ts.tv_nsec;
            buttons_port_.write();
            break;
        default:
            // Ignore
            break;
    }

    return RTC::RTC_OK;
}


static const char* spec[] =
{
    "implementation_id", "RTCJoy",
    "type_name",         "rtcjoy",
    "description",       "UNIX Joystick component.",
    "version",           "1.0",
    "vendor",            "Geoffrey Biggs, AIST",
    "category",          "Hardware",
    "activity_type",     "PERIODIC",
    "kind",              "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",
    // Configuration variables
    "conf.default.device", "/dev/input/js0",
    "conf.default.x_axis", "0",
    "conf.default.y_axis", "1",
    "conf.default.scale_v", "-1.0",
    "conf.default.scale_a", "1.0",
#if defined(RTDOC_SUPPORT)
    "conf.__description__.device", "Path of the joystick device node.",
    "conf.__description__.x_axis", "Axis index to use for the X axis.",
    "conf.__description__.y_axis", "Axis index to use for the Y axis.",
    "conf.__description__.scale_v",
        "Scale factor to convert Y position into velocity in m/s.",
    "conf.__description__.scale_a",
        "Scale factor to convert X position into angular velocity in rad/s.",
#endif //defined(RTDOC_SUPPORT)
    // Widget
    "conf.__widget__.device", "text",
    "conf.__widget__.x_axis", "spin",
    "conf.__widget__.y_axis", "spin",
    "conf.__widget__.scale_v", "spin",
    "conf.__widget__.scale_a", "spin",
    // Constraints
    "conf.__constraints__.x_axis", "0<=x",
    "conf.__constraints__.y_axis", "0<=x",
    "conf.__constraints__.scale_v", "0<=x",
    "conf.__constraints__.scale_a", "0<=x",
    // Documentation
#if defined(RTDOC_SUPPORT)
    //"conf.__doc__.__order__", "",
    "conf.__doc__.__license__", "EPL",
    "conf.__doc__.__contact__", "geoffrey.biggs@aist.go.jp",
    "conf.__doc__.__url__", "https://www.openrtm.org",
    "conf.__doc__.intro",
        "An RT-Component for UNIX joystick devices, such as those exposed via /dev/js* nodes.",
    "conf.__doc__.reqs", "None.",
    "conf.__doc__.install",
        "Use CMake to build the source, then run 'make install'.",
    "conf.__doc__.usage",
        "Launch using rtcjoy_standalone for a stand-alone component. Use librtcjoy.so with managers.",
    //"conf.__doc__.misc", "",
    //"conf.__doc__.changelog", "",
#endif //defined(RTDOC_SUPPORT)
    ""
};

extern "C"
{
    void rtc_init(RTC::Manager* manager)
    {
        coil::Properties profile(spec);
        manager->registerFactory(profile, RTC::Create<RTCJoy>,
                RTC::Delete<RTCJoy>);
    }
};

