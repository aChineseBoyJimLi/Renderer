#pragma once

#include "RHIDefinitions.h"

class RHICommandList : public RHIObject
{
public:
    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual bool IsClosed() const = 0;
    virtual ERHICommandQueueType GetQueueType() const = 0;
};