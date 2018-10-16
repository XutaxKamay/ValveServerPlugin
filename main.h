#ifndef MAIN_DLL
#define MAIN_DLL

#include "headers.h"

#define FORCEINLINE inline

#if __x86_64__ || __ppc64__
#define MX64
#else
#define MX86
#endif

#define InvalidQueryCvarCookie -1

// CHANGE IT WITH THE VERSION OF YOUR GAME! This is currently for CS:GO.
#define INTERFACEVERSION_ISERVERPLUGINCALLBACKS				"ISERVERPLUGINCALLBACKS004"

enum
{
    IFACE_OK = 0 ,
    IFACE_FAILED
};

//
// you will also want to listen for game events via IGameEventManager::AddListener()
//

typedef enum
{
    PLUGIN_CONTINUE = 0 , // keep going
    PLUGIN_OVERRIDE , // run the game dll function but use our return value instead
    PLUGIN_STOP , // don't run the game dll function at all
} PLUGIN_RESULT;


typedef enum
{
    eQueryCvarValueStatus_ValueIntact = 0 ,	// It got the value fine.
    eQueryCvarValueStatus_CvarNotFound = 1 ,
    eQueryCvarValueStatus_NotACvar = 2 ,		// There's a ConCommand, but it's not a ConVar.
    eQueryCvarValueStatus_CvarProtected = 3	// The cvar was marked with FCVAR_SERVER_CAN_NOT_QUERY, so the server is not allowed to have its value.
} EQueryCvarValueStatus;

class CCommand;
struct edict_t;
class KeyValues;

typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t byte;
typedef void* Pointer;
typedef unsigned char BYTE;
typedef int QueryCvarCookie_t;

struct Vector
{
    float x , y , z;
};

struct QAngle
{
    float x , y , z;
};

template<typename T = Pointer> bool IsPointer( T pP )
{
    return reinterpret_cast< Pointer >( pP ) != nullptr;
}

template<typename T = Pointer*>
FORCEINLINE auto sub_GetVTable( Pointer pAddress ) -> T
{
    return *reinterpret_cast< T* >( pAddress );
}

typedef void* ( *CreateInterfaceFn )( const char *pName , int *pReturnCode );
typedef void* ( *InstantiateInterfaceFn )( );

#define  DLL_EXPORT   extern "C" __attribute__ ((visibility("default")))
#define  DLL_IMPORT   extern "C"

#define  DLL_CLASS_EXPORT __attribute__ ((visibility("default")))
#define  DLL_CLASS_IMPORT

#define  DLL_GLOBAL_EXPORT   extern __attribute ((visibility("default")))
#define  DLL_GLOBAL_IMPORT   extern

#define  DLL_LOCAL __attribute__ ((visibility("hidden")))

// Use this to expose an interface that can have multiple instances.
// e.g.:
// EXPOSE_INTERFACE( CInterfaceImp, IInterface, "MyInterface001" )
// This will expose a class called CInterfaceImp that implements IInterface (a pure class)
// clients can receive a pointer to this class by calling CreateInterface( "MyInterface001" )
//
// In practice, the shared header file defines the interface (IInterface) and version name ("MyInterface001")
// so that each component can use these names/vtables to communicate
//
// A single class can support multiple interfaces through multiple inheritance
//
// Use this if you want to write the factory function.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	static InterfaceReg __g_Create##interfaceName##_reg(functionName, versionName);
#else
#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static InterfaceReg __g_Create##interfaceName##_reg(functionName, versionName); \
	}
#endif

#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	static void* __Create##className##_interface() {return static_cast<interfaceName *>( new className );} \
	static InterfaceReg __g_Create##className##_reg(__Create##className##_interface, versionName );
#else
#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static void* __Create##className##_interface() {return static_cast<interfaceName *>( new className );} \
		static InterfaceReg __g_Create##className##_reg(__Create##className##_interface, versionName ); \
	}
#endif

// Use this to expose a singleton interface with a global variable you've created.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR_WITH_NAMESPACE(className, interfaceNamespace, interfaceName, versionName, globalVarName) \
	static void* __Create##className##interfaceName##_interface() {return static_cast<interfaceNamespace interfaceName *>( &globalVarName );} \
	static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName);
#else
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR_WITH_NAMESPACE(className, interfaceNamespace, interfaceName, versionName, globalVarName) \
	namespace _SUBSYSTEM \
	{ \
		static void* __Create##className##interfaceName##_interface() {return static_cast<interfaceNamespace interfaceName *>( &globalVarName );} \
		static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName); \
	}
#endif

#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName) \
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR_WITH_NAMESPACE(className, , interfaceName, versionName, globalVarName)

// Use this to expose a singleton interface. This creates the global variable for you automatically.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	static className __g_##className##_singleton; \
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, __g_##className##_singleton)
#else
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static className __g_##className##_singleton; \
	}	\
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, __g_##className##_singleton)
#endif

// Used internally to register classes.
class InterfaceReg
{
public:
    InterfaceReg( InstantiateInterfaceFn fn , const char *pName );

public:
    InstantiateInterfaceFn	m_CreateFn;
    const char				*m_pName;

    InterfaceReg			*m_pNext; // For the global list.
    static InterfaceReg		*s_pInterfaceRegs;
};

class IGameEventListener
{
public:
    virtual	~IGameEventListener( void )
    {
    };

    // FireEvent is called by EventManager if event just occured
    // KeyValue memory will be freed by manager if not needed anymore
    virtual void FireGameEvent( KeyValues * event ) = 0;
};


class IServerPluginCallbacks
{
public:
    // Initialize the plugin to run
    // Return false if there is an error during startup.
    virtual bool			Load( CreateInterfaceFn interfaceFactory , CreateInterfaceFn gameServerFactory ) = 0;

    // Called when the plugin should be shutdown
    virtual void			Unload( void ) = 0;

    // called when a plugins execution is stopped but the plugin is not unloaded
    virtual void			Pause( void ) = 0;

    // called when a plugin should start executing again (sometime after a Pause() call)
    virtual void			UnPause( void ) = 0;

    // Returns string describing current plugin.  e.g., Admin-Mod.  
    virtual const char     *GetPluginDescription( void ) = 0;

    // Called any time a new level is started (after GameInit() also on level transitions within a game)
    virtual void			LevelInit( char const *pMapName ) = 0;

    // The server is about to activate
    virtual void			ServerActivate( edict_t *pEdictList , int edictCount , int clientMax ) = 0;

    // The server should run physics/think on all edicts
    virtual void			GameFrame( bool simulating ) = 0;

    // Called when a level is shutdown (including changing levels)
    virtual void			LevelShutdown( void ) = 0;

    // Client is going active
    virtual void			ClientActive( edict_t *pEntity ) = 0;

    // Client is fully connected ( has received initial baseline of entities )
    virtual void			ClientFullyConnect( edict_t *pEntity ) = 0;

    // Client is disconnecting from server
    virtual void			ClientDisconnect( edict_t *pEntity ) = 0;

    // Client is connected and should be put in the game
    virtual void			ClientPutInServer( edict_t *pEntity , char const *playername ) = 0;

    // Sets the client index for the client who typed the command into their console
    virtual void			SetCommandClient( int index ) = 0;

    // A player changed one/several replicated cvars (name etc)
    virtual void			ClientSettingsChanged( edict_t *pEdict ) = 0;

    // Client is connecting to server ( set retVal to false to reject the connection )
    //	You can specify a rejection message by writing it into reject
    virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect , edict_t *pEntity , const char *pszName , const char *pszAddress , char *reject , int maxrejectlen ) = 0;

    // The client has typed a command at the console
    virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity , const CCommand &args ) = 0;

    // A user has had their network id setup and validated 
    virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName , const char *pszNetworkID ) = 0;

    // This is called when a query from IServerPluginHelpers::StartQueryCvarValue is finished.
    // iCookie is the value returned by IServerPluginHelpers::StartQueryCvarValue.
    // Added with version 2 of the interface.
    virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie , edict_t *pPlayerEntity , EQueryCvarValueStatus eStatus , const char *pCvarName , const char *pCvarValue ) = 0;

    // added with version 3 of the interface.
    virtual void			OnEdictAllocated( edict_t *edict ) = 0;
    virtual void			OnEdictFreed( const edict_t *edict ) = 0;

    //
    // Allow plugins to validate and configure network encryption keys (added in Version 4 of the interface)
    // Game server must run with -externalnetworkcryptkey flag, and 3rd party client software must set the
    // matching encryption key in the client game process.
    //

    // BNetworkCryptKeyCheckRequired allows the server to allow connections from clients or relays that don't have
    // an encryption key. The function must return true if the client encryption key is required, and false if the client
    // is allowed to connect without an encryption key. It is recommended that if client wants to use encryption key
    // this function should return true to require it on the server side as well.
    // Any plugin in the chain that returns true will flag connection to require encryption key for the engine and check
    // with other plugins will not be continued.
    // If no plugin returns true to require encryption key then the default implementation will require encryption key
    // if the client wants to use it.
    virtual bool			BNetworkCryptKeyCheckRequired( uint32 unFromIP , uint16 usFromPort , uint32 unAccountIdProvidedByClient ,
                                                           bool bClientWantsToUseCryptKey ) = 0;

    // BNetworkCryptKeyValidate allows the server to validate client's over the wire encrypted payload cookie and return
    // false if the client cookie is malformed to prevent connection to the server. If this function returns true then
    // the plugin allows the client to connect with the encryption key, and upon return the engine expects the plugin
    // to have copied 16-bytes of client encryption key into the buffer pointed at by pbPlainTextKeyForNetChan. That key
    // must match the plaintext key supplied by 3rd party client software to the client game process, not the client cookie
    // transmitted unencrypted over the wire as part of the connection packet.
    // Any plugin in the chain that returns true will stop evaluation of other plugins and the 16-bytes encryption key
    // copied into pbPlainTextKeyForNetchan will be used. If a plugin returns false then evaluation of other plugins will
    // continue and buffer data in pbPlainTextKeyForNetchan will be preserved from previous calls.
    // If no plugin returns true and the encryption key is required then the client connection will be rejected with
    // an invalid certificate error.
    virtual bool			BNetworkCryptKeyValidate( uint32 unFromIP , uint16 usFromPort , uint32 unAccountIdProvidedByClient ,
                                                      int nEncryptionKeyIndexFromClient , int numEncryptedBytesFromClient , byte *pbEncryptedBufferFromClient ,
                                                      byte *pbPlainTextKeyForNetchan ) = 0;
};

class CSamplePlugin : public IServerPluginCallbacks , public IGameEventListener
{
    // IServerPluginCallbacks methods
public:

    CSamplePlugin()
    {
        m_iClientCommandIndex = 0;
    }

    ~CSamplePlugin()
    {

    }

    virtual bool			Load( CreateInterfaceFn interfaceFactory , CreateInterfaceFn gameServerFactory );
    virtual void			Unload( void );
    virtual void			Pause( void )
    {
    };
    virtual void			UnPause( void )
    {
    };
    virtual const char		*GetPluginDescription( void )
    {
        return "Sample plugin.";
    };
    virtual void			LevelInit( char const *pMapName )
    {
    };
    virtual void			ServerActivate( edict_t *pEdictList , int edictCount , int clientMax )
    {
    };
    virtual void			GameFrame( bool simulating )
    {
    };
    virtual void			LevelShutdown( void )
    {
    };
    virtual void			ClientActive( edict_t *pEntity )
    {
    };
    virtual void			ClientFullyConnect( edict_t *pEntity )
    {
    };
    virtual void			ClientDisconnect( edict_t *pEntity )
    {
    };
    virtual void			ClientPutInServer( edict_t *pEntity , char const *playername )
    {
    };
    virtual void			SetCommandClient( int index )
    {
    };
    virtual void			ClientSettingsChanged( edict_t *pEdict )
    {
    };
    virtual PLUGIN_RESULT	ClientConnect( bool *bAllowConnect , edict_t *pEntity , const char *pszName , const char *pszAddress , char *reject , int maxrejectlen )
    {
        return PLUGIN_CONTINUE;
    };
    virtual PLUGIN_RESULT	ClientCommand( edict_t *pEntity , const CCommand &args )
    {
        return PLUGIN_CONTINUE;
    };
    virtual PLUGIN_RESULT	NetworkIDValidated( const char *pszUserName , const char *pszNetworkID )
    {
        return PLUGIN_CONTINUE;
    };
    virtual void			OnQueryCvarValueFinished( QueryCvarCookie_t iCookie , edict_t *pPlayerEntity , EQueryCvarValueStatus eStatus , const char *pCvarName , const char *pCvarValue )
    {
    };
    virtual void			OnEdictAllocated( edict_t *edict )
    {
    };
    virtual void			OnEdictFreed( const edict_t *edict )
    {
    };
    virtual void			FireGameEvent( KeyValues *event )
    {
    };
    virtual bool			BNetworkCryptKeyCheckRequired( uint32 unFromIP , uint16 usFromPort , uint32 unAccountIdProvidedByClient ,
                                                   bool bClientWantsToUseCryptKey )
    {
        return true;
    }
    virtual bool			BNetworkCryptKeyValidate( uint32 unFromIP , uint16 usFromPort , uint32 unAccountIdProvidedByClient ,
                                              int nEncryptionKeyIndexFromClient , int numEncryptedBytesFromClient , byte *pbEncryptedBufferFromClient ,
                                              byte *pbPlainTextKeyForNetchan )
    {
        return true;
    }
    int						GetCommandIndex()
    {
        return m_iClientCommandIndex;
    }

private:
    int m_iClientCommandIndex;
};


// 64 bits
/*
0:  48 b8 45 55 46 84 45    movabs rax,0x454584465545
7:  45 00 00
a:  ff e0                   jmp    rax
*/

// 32 bits
/*
0:  b8 01 00 00 00          mov    eax,0x1
5:  ff e0                   jmp    eax
*/

class CDetour
{
public:

    CDetour( Pointer pFunction , Pointer pNewFunction )
        : m_pFunction( nullptr ) , m_pNewFunction( nullptr ) , m_SavedBytes { 0 } , m_PageSize( 0 ) , m_PageStart( 0 )
#ifdef MX64
        , m_LongJmp { 0x48 , 0xB8 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0x00 , 0xFF , 0xE0 }
#else
        , m_LongJmp { 0xB8 , 0x00 , 0x00 , 0x00 , 0x00 , 0xFF , 0xE0 }
#endif
    {
        m_pFunction = pFunction;
        m_pNewFunction = pNewFunction;
        m_SavedBytes.resize( m_LongJmp.size() );

        m_PageSize = ( unsigned long ) sysconf( _SC_PAGE_SIZE );

        m_PageStart = reinterpret_cast< uintptr_t >( m_pFunction ) & -m_PageSize;

        memcpy( m_SavedBytes.data() , m_pFunction , m_SavedBytes.size() );
#ifdef MX64
        memcpy( reinterpret_cast< Pointer >( reinterpret_cast< uintptr_t >( m_LongJmp.data() ) + 2 ) , &m_pNewFunction , sizeof( Pointer ) );
#else
        memcpy( reinterpret_cast< Pointer >( reinterpret_cast< uintptr_t >( m_LongJmp.data() ) + 1 ) , &m_pNewFunction , sizeof( Pointer ) );
#endif
    }

    FORCEINLINE auto IsValid() -> bool
    {
        return IsPointer( m_pFunction ) && IsPointer( m_pNewFunction );
    }

    auto Detour() -> bool
    {
        if ( !IsValid() )
            return false;

        if ( mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_WRITE | PROT_EXEC ) != -1 )
        {
            memcpy( m_pFunction , m_LongJmp.data() , m_LongJmp.size() );

            // We assume that it was read + exec.
            return mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_EXEC ) != -1;
        }

        return false;
    }

    auto Reset() -> bool
    {
        if ( !IsValid() )
            return false;

        if ( mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_WRITE | PROT_EXEC ) != -1 )
        {
            memcpy( m_pFunction , m_SavedBytes.data() , m_SavedBytes.size() );

            // We assume that it was read + exec.
            return mprotect( reinterpret_cast< Pointer >( m_PageStart ) , m_PageSize , PROT_READ | PROT_EXEC ) != -1;
        }

        return false;
    }

    FORCEINLINE auto GetFunction() -> Pointer
    {
        return m_pFunction;
    }

    FORCEINLINE auto GetNewFunction() -> Pointer
    {
        return m_pNewFunction;
    }

    template< typename RetType = void , typename ... vArgs >
    FORCEINLINE auto _CallFunction( vArgs ... pArgs ) -> RetType
    {
        return reinterpret_cast< RetType( *)( vArgs ... ) >( m_pFunction ) ( pArgs ... );
    }

    template< typename RetType = void , typename ... vArgs >
    FORCEINLINE auto CallFunction( vArgs ... pArgs ) -> RetType
    {
        if ( Reset() )
        {
            _CallFunction<RetType , vArgs...>( pArgs... );
            Detour();
        }
    }

    ~CDetour()
    {
        Reset();
        m_LongJmp.clear();
        m_SavedBytes.clear();
        m_pFunction = nullptr;
        m_pNewFunction = nullptr;
    }

private:
    Pointer m_pFunction , m_pNewFunction;
    std::vector<BYTE> m_SavedBytes;
    unsigned long m_PageSize;
    uintptr_t m_PageStart;
    std::vector<BYTE> m_LongJmp;

};

typedef CDetour* pCDetour;

#endif
