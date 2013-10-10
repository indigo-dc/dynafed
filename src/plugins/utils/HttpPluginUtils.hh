#ifndef HTTPPLUGINUTILS_HPP
#define HTTPPLUGINUTILS_HPP

#include <cstring>
#include "PluginUtils.hh"
#include <davix.hpp>


namespace HttpUtils{

enum CredType{
    ProxyCred,
    PemCred,
    Pkcs12Cred
};

inline CredType parseCredType(const std::string & cred_type){
    if(strcasecmp(cred_type.c_str(), "PEM") ==0)
        return PemCred;
    if(strcasecmp(cred_type.c_str(), "proxy") ==0)
        return ProxyCred;
    // default pkcs12
    return Pkcs12Cred;
}

/**  ssl_check : TRUE | FALSE   - enable or disable the CA check for the server certificate
*
*  cli_private_key : path      - path to the private key to use for this endpoint
*  cli_certificate : path      - path to the credential to use for this endpoint
*  cli_password : password     - password to use for this credential
*  auth_login : login		   - login to use for basic HTTP authentification
*  auth_passwd : password	   - password to use for the basic HTTP authentification
* */
inline void configureSSLParams(const std::string & plugin_name,
                              const std::string & prefix,
                              Davix::RequestParams & params){

    Davix::DavixError * tmp_err = NULL;
    Davix::X509Credential cred;
    int ret = 0;

    // get ssl check
    const bool ssl_check = pluginGetParam<bool>(prefix, "ssl_check", true);
    Info(SimpleDebug::kLOW, plugin_name, "SSL CA check for davix is set to  " + std::string((ssl_check) ? "TRUE" : "FALSE"));
    params.setSSLCAcheck(ssl_check);
    // ca check
    const std::string ca_path = pluginGetParam<std::string>(prefix, "ca_path");
    if( ca_path.size() > 0){
        Info(SimpleDebug::kLOW, plugin_name, "CA Path added :  " << ca_path);
        params.addCertificateAuthorityPath(ca_path);
    }

    // setup cli type
    const std::string credential_type_str = pluginGetParam<std::string>(prefix, "cli_type", "pkcs12");
    const CredType credential_type = parseCredType(credential_type_str);
    if(credential_type != Pkcs12Cred)
        Info(SimpleDebug::kLOW, plugin_name, " CLI cert type defined to " << credential_type);
    // setup private key
    const std::string private_key_path = pluginGetParam<std::string>(prefix, "cli_private_key");
    if (private_key_path.size() > 0)
        Info(SimpleDebug::kLOW, plugin_name, " CLI priv key defined");
    // setup credential
    const std::string credential_path = pluginGetParam<std::string>(prefix, "cli_certificate");
    if (credential_path.size() > 0)
        Info(SimpleDebug::kLOW, plugin_name, " CLI CERT path is set to " + credential_path);
    // setup credential password
    const std::string credential_password = pluginGetParam<std::string>(prefix, "cli_password");
    if (credential_password.size() > 0)
        Info(SimpleDebug::kLOW, plugin_name, " CLI CERT password defined");

    if (credential_path.size() > 0) {
        switch(credential_type){
            case ProxyCred:
            case PemCred:
                ret= cred.loadFromFilePEM(private_key_path, credential_path, credential_password, &tmp_err);
            break;
            default:
                ret= cred.loadFromFileP12(credential_path, credential_password, &tmp_err);
        }
        if(ret >= 0){
            params.setClientCertX509(cred);
        }else {
            Info(SimpleDebug::kHIGH, plugin_name, "Error: impossible to load credential "
                    + credential_path + " :" + tmp_err->getErrMsg());
            Davix::DavixError::clearError(&tmp_err);
        }
     }

}


inline void configureHttpAuth(const std::string & plugin_name,
                              const std::string & prefix,
                              Davix::RequestParams & params){

    // auth login
    const std::string login = pluginGetParam<std::string>(prefix, "auth_login");
    // auth password
    const std::string password = pluginGetParam<std::string>(prefix, "auth_passwd");
    if (password.size() > 0 && login.size() > 0) {
        Info(SimpleDebug::kLOW, plugin_name, "login and password setup for authentication");
        params.setClientLoginPassword(login, password);
    }
}


inline void configureHttpTimeout(const std::string & plugin_name,
                                 const std::string & prefix,
                                 Davix::RequestParams & params){
    // timeout management
    long timeout;
    struct timespec spec_timeout;
    if ((timeout =pluginGetParam<long>(prefix, "conn_timeout", 120)) != 0) {
        Info(SimpleDebug::kLOW, plugin_name, "Connection timeout is set to : " << timeout);
        spec_timeout.tv_sec = timeout;
        params.setConnectionTimeout(&spec_timeout);
    }
    if ((timeout = pluginGetParam<long>(prefix, "ops_timeout", 120)) != 0) {
        spec_timeout.tv_sec = timeout;
        params.setOperationTimeout(&spec_timeout);
        Info(SimpleDebug::kLOW, plugin_name, "Operation timeout is set to : " << timeout);
    }
}


inline std::string protocolHttpNormalize(const std::string & url){
    if(url.compare(0,4,"http") == 0)
        return url;
    std::string res = url;
    std::string::iterator it = std::find(res.begin(),res.end(), ':');
    if( it == res.end())
        return res;
    if( it > res.begin() && *(it-1) == 's')
        res.replace(res.begin(), it, "https");
    else
        res.replace(res.begin(), it, "http");
    return res;
}

}


#endif // HTTPPLUGINUTILS_HPP
