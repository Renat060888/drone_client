
#ifdef OBJREPR_LIBRARY_EXIST
#include <objrepr/reprServer.h>
#endif

#include "common/ms_common_utils.h"
#include "common/ms_common_vars.h"
#include "system/logger.h"

#include "objrepr_bus.h"

using namespace std;

bool ObjreprBus::m_systemInited = false;

ObjreprBus::ObjreprBus()
{

}

ObjreprBus::~ObjreprBus()
{
    shutdown();
}

bool ObjreprBus::init( const SInitSettings & _settings ){

    if( m_systemInited ){
        return true;
    }

    VS_LOG_INFO << "objrepr system-init begin" << endl;

    if( ! launch(_settings.objreprConfigPath) ){
        assert( false && "objrepr launch crashed :(" );
    }

    if( ! _settings.initialContextName.empty() ){

        const common_types::TContextId ctxId = getContextIdByName( _settings.initialContextName );
        if( ! openContext(ctxId) ){
            assert( false && "objrepr opening context crashed :(" );
        }        
    }

    VS_LOG_INFO << "objrepr system-init success" << endl;

    m_systemInited = true;
    return true;
}

void ObjreprBus::shutdown(){

    shutdownDerive();

#ifdef OBJREPR_LIBRARY_EXIST
    if( m_systemInited ){
        assert( objrepr::RepresentationServer::instance()->state() != objrepr::RepresentationServer::State::ST_Stopped );

        m_systemInited = false;
    }
#endif
}

bool ObjreprBus::launch( const std::string & _configPath ){
#ifdef OBJREPR_LIBRARY_EXIST
    const bool configured = objrepr::RepresentationServer::instance()->configure( _configPath.c_str() );
    if( ! configured ){
        VS_LOG_CRITICAL << "objrepr Can't configure by: [1], reason: [2] " << _configPath << " " << objrepr::RepresentationServer::instance()->errString() << endl;
        return false;
    }

    const bool launched = objrepr::RepresentationServer::instance()->launch();
    if( ! launched ){
        VS_LOG_CRITICAL << "objrepr Can't launch, reason: " << objrepr::RepresentationServer::instance()->errString() << endl;
        return false;
    }
#endif
    return true;
}

bool ObjreprBus::openContext( common_types::TContextId _ctxId ){
#ifdef OBJREPR_LIBRARY_EXIST
    const bool opened = objrepr::RepresentationServer::instance()->setCurrentContext( _ctxId );
    if( ! opened ){
        m_lastError = objrepr::RepresentationServer::instance()->errString();
        VS_LOG_CRITICAL << "objrepr context open fail, reason: " << objrepr::RepresentationServer::instance()->errString() << endl;
        return false;
    }

    VS_LOG_INFO << "================ OBJREPR CONTEXT OPENED: [" << objrepr::RepresentationServer::instance()->currentContext()->name()
             << "] ================"
             << endl;
#endif
    return true;
}

bool ObjreprBus::openContextAsync( common_types::TContextId _ctxId ){

    m_futureObjreprContextLoading = std::async( std::launch::async, & ObjreprBus::threadObjreprContextLoading, this, _ctxId );
    return true;
}

common_types::TContextId ObjreprBus::getCurrentContextId(){
#ifdef OBJREPR_LIBRARY_EXIST
    return objrepr::RepresentationServer::instance()->currentContext()->id();
#else
    return 0;
#endif
}

bool ObjreprBus::closeContext(){
#ifdef OBJREPR_LIBRARY_EXIST
    if( ! objrepr::RepresentationServer::instance()->currentContext() ){
        stringstream ss;
        ss << "objrepr context open fail, nothing to close";
        m_lastError = ss.str();
        VS_LOG_CRITICAL << ss.str() << endl;
        return false;
    }

    const bool opened = objrepr::RepresentationServer::instance()->setCurrentContext( (uint32_t)0 );
    assert( opened );
#endif
    return true;
}

common_types::TContextId ObjreprBus::getContextIdByName( const std::string & _ctxName ){
#ifdef OBJREPR_LIBRARY_EXIST
    common_types::TContextId contextId = common_vars::INVALID_CONTEXT_ID;
    std::vector<objrepr::ContextPtr> contexts = objrepr::RepresentationServer::instance()->contextList();
    for( objrepr::ContextPtr & ctx : contexts ){
        if( ctx->name() == _ctxName ){
            contextId = ctx->id();
        }
    }

    return contextId;
#else
    return 0;
#endif
}

std::string ObjreprBus::getContextNameById( common_types::TContextId _ctxId ){
#ifdef OBJREPR_LIBRARY_EXIST
    string contextName;
    std::vector<objrepr::ContextPtr> contexts = objrepr::RepresentationServer::instance()->contextList();
    for( objrepr::ContextPtr & ctx : contexts ){
        if( ctx->id() == _ctxId ){
            contextName = ctx->name();
        }
    }

    return contextName;
#else
    return string();
#endif
}

void ObjreprBus::threadObjreprContextLoading( common_types::TContextId _ctxId ){
#ifdef OBJREPR_LIBRARY_EXIST
    const bool opened = objrepr::RepresentationServer::instance()->setCurrentContext( _ctxId );
    if( ! opened ){
        m_lastError = objrepr::RepresentationServer::instance()->errString();
        VS_LOG_CRITICAL << "objrepr context open fail, reason: " << objrepr::RepresentationServer::instance()->errString() << endl;
    }
#endif
}







