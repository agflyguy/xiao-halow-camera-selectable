#pragma once

#include "CStreamer.h"

class EspCameraStreamer : public CStreamer
{
public:
    EspCameraStreamer(u_short width, u_short height);

    void streamImage(uint32_t curMsec) override;
};
