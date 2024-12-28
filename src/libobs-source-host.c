#include "libobs-source-host.h"

int main()
{
    obs_startup("en_US", NULL, NULL);
    // TODO: do stuff
    obs_shutdown();
    return 0;
}
