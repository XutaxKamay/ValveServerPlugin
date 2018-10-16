#include "main.h"

CSamplePlugin g_SamplePlugin;

InterfaceReg *InterfaceReg::s_pInterfaceRegs = nullptr;

InterfaceReg::InterfaceReg( InstantiateInterfaceFn fn , const char *pName ) : m_pName( pName )
{
    m_CreateFn = fn;
    m_pNext = s_pInterfaceRegs;
    s_pInterfaceRegs = this;
}

void* CreateInterfaceInternal( const char *pName , int *pReturnCode )
{
    InterfaceReg *pCur;

    for ( pCur = InterfaceReg::s_pInterfaceRegs; pCur; pCur = pCur->m_pNext )
    {
        if ( strcmp( pCur->m_pName , pName ) == 0 )
        {
            if ( pReturnCode )
            {
                *pReturnCode = IFACE_OK;
            }
            return pCur->m_CreateFn();
        }
    }

    if ( pReturnCode )
    {
        *pReturnCode = IFACE_FAILED;
    }

    return NULL;
}

DLL_EXPORT void* CreateInterface( const char *pName , int *pReturnCode )
{
    return CreateInterfaceInternal( pName , pReturnCode );
}
             
void CSamplePlugin::Unload()
{
    printf( "Unload...\n" );
}

bool CSamplePlugin::Load( CreateInterfaceFn interfaceFactory , CreateInterfaceFn gameServerFactory )
{
    printf( "Load...\n" );

    auto lm_Server = ( link_map* ) dlopen( "csgo/bin/server.so" , RTLD_NOW );

    if ( lm_Server == nullptr )
    {
        printf( "Failed to get server library...\n" );
        return false;
    }

    printf( "%p\n" , reinterpret_cast< Pointer >( lm_Server->l_addr ) );

    return true;
}

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CSamplePlugin , IServerPluginCallbacks , INTERFACEVERSION_ISERVERPLUGINCALLBACKS , g_SamplePlugin );
