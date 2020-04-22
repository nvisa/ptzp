QT -= gui
QT += network

DEFINES += DEBUG INFO LOG LOGV DEBUG_TIMING
TARGET = ptzp
TEMPLATE = lib
CONFIG += c++11 staticlib
DEFINES += QESP_NO_UDEV

include (ptzp.pri)

SOURCES += \
        presetng.cpp \
        flirpthead.cpp \
        aryadriver.cpp \
        mgeothermalhead.cpp \
        aryapthead.cpp \
        qextserialport/qextserialenumerator_linux.cpp \
        qextserialport/qextserialport_unix.cpp \
        qextserialport/qextserialport.cpp \
        qextserialport/qextserialenumerator.cpp \
        yamgozdriver.cpp \
        mgeoswirhead.cpp \
        mgeoyamgozhead.cpp \
        ptzpserialtransport.cpp \
        flirdriver.cpp \
        hardwareoperations.cpp \
        networkcommon.cpp \
        exarconfig.cpp \
        swirdriver.cpp \
        ptz365driver.cpp \
        ptzptcptransport.cpp \
        remoteconnection.cpp \
        thermalmoduledriver.cpp \
        ptzpdriver.cpp \
        patrolng.cpp \
        ptzphttptransport.cpp \
        virtualptzpdriver.cpp \
        oem4kdriver.cpp \
        patternng.cpp \
        evpupthead.cpp \
        kayidriver.cpp \
        debug.cpp \
        gpiocontroller.cpp \
        simplehttpserver.cpp \
        gungorhead.cpp \
        remotetcpconnection.cpp \
        oemmodulehead.cpp \
        ptzphead.cpp \
        tbgthdriver.cpp \
        multicastsocket.cpp \
        flirpttcphead.cpp \
        i2cdevice.cpp \
        htrswirpthead.cpp \
        moxacontrolhead.cpp \
        irdomedriver.cpp \
        flirmodulehead.cpp \
        hitachimodule.cpp \
        oem4kmodulehead.cpp \
        yamanolenshead.cpp \
        systeminfo.cpp \
        htrswirmodulehead.cpp \
        htrswirdriver.cpp \
        ptzptransport.cpp \
        irdomepthead.cpp \
        mgeofalconeyehead.cpp \
        simplemetrics.cpp \
    mgeodortgozhead.cpp \
dortgozdriver.cpp

HEADERS += \
        i2cdevice.h \
        oemmodulehead.h \
        remoteconnection.h \
        ptzphead.h \
        qextserialport/qextserialport_p.h \
        qextserialport/qextserialenumerator_p.h \
        qextserialport/qextserialport.h \
        qextserialport/qextserialenumerator.h \
        qextserialport/qextserialport_global.h \
        oem4kmodulehead.h \
        patrolng.h \
        mgeoyamgozhead.h \
        oem4kdriver.h \
        ptzphttptransport.h \
        ptzpatterninterface.h \
        flirpthead.h \
        remotetcpconnection.h \
        mgeoswirhead.h \
        ptzpdriver.h \
        yamanolenshead.h \
        systeminfo.h \
        kayidriver.h \
        swirdriver.h \
        ptz365driver.h \
        moxacontrolhead.h \
        thermalmoduledriver.h \
        exarconfig.h \
        simplehttpserver.h \
        gungorhead.h \
        ptzpserialtransport.h \
        ptzptransport.h \
        aryapthead.h \
        hardwareoperations.h \
        irdomedriver.h \
        ptzptcptransport.h \
        networkcommon.h \
        networkmanagementinterface.h \
        irdomepthead.h \
        virtualptzpdriver.h \
        keyvalueinterface.h \
        flirpttcphead.h \
        aryadriver.h \
        debug.h \
        onvifdevicemgtsysteminterface.h \
        flirmodulehead.h \
        flirdriver.h \
        htrswirmodulehead.h \
        htrswirdriver.h \
        patternng.h \
        ptzcontrolinterface.h \
        multicastsocket.h \
        evpupthead.h \
        presetng.h \
        htrswirpthead.h \
        mgeothermalhead.h \
        ioerrors.h \
        mgeofalconeyehead.h \
        yamgozdriver.h \
        systemtimeinterface.h \
        hitachimodule.h \
        gpiocontroller.h \
        tbgthdriver.h \ 
        simplemetrics.h \
	mgeodortgozhead.h \
	dortgozdriver.h

contains(QT, websockets) {
    SOURCES += networksource.cpp
    HEADERS += networksource.h
}

deprecated {
    HEADERS += \
        viscamodule.h \

    SOURCES += \
        viscamodule.cpp \


}
#Add make targets for checking version info
VersionCheck.commands = @$$PWD/checkversion.sh $$PWD
QMAKE_EXTRA_TARGETS += VersionCheck
PRE_TARGETDEPS += VersionCheck
