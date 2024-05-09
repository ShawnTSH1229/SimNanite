#include "SimNaniteGlobalResource.h"

static SSimNaniteGlobalResource gSimNaniteResource;

SSimNaniteGlobalResource& GetSimNaniteGlobalResource()
{
    return gSimNaniteResource;
}
