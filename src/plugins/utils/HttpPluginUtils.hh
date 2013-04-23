#ifndef HTTPPLUGINUTILS_HPP
#define HTTPPLUGINUTILS_HPP

#include "PluginUtils.hh"
#include <davix.hpp>


namespace HttpUtils{

inline void configureSSLParams(const std::string & plugin_name,
                              const std::string & prefix,
                              Davix::RequestParams & params){

    Davix::DavixError * tmp_err = NULL;
    Davix::X509Credential cred;

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
    // get credential
    const std::string pkcs12_credential_path = pluginGetParam<std::string>(prefix, "cli_certificate");
    // get credential password
    const std::string pkcs12_credential_password = pluginGetParam<std::string>(prefix, "cli_password");
    if (pkcs12_credential_path.size() > 0) {
        Info(SimpleDebug::kLOW, plugin_name, " CLI CERT path is set to  " + pkcs12_credential_path);
        if (pkcs12_credential_password.size() > 0)
            Info(SimpleDebug::kLOW, plugin_name, " CLI CERT password defined  ");
        if (cred.loadFromFileP12(pkcs12_credential_path, pkcs12_credential_password, &tmp_err) >= 0) {
            params.setClientCertX509(cred);
        }else {
            Info(SimpleDebug::kHIGH, plugin_name, "Error: impossible to load credential "
                    + pkcs12_credential_path + " :" + tmp_err->getErrMsg());
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

}


#endif // HTTPPLUGINUTILS_HPP
