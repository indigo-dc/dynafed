# @brief cmake define file for apache configuration environment
#
# @author Adrien Devresse


include(DefineInstallationPaths REQUIRED)


SET(APACHE_CONF_DIR "${SYSCONF_INSTALL_DIR}/apache2/conf.d/")
SET(HTTPD_CONF_DIR "${SYSCONF_INSTALL_DIR}/httpd/conf.d/")

message(STATUS " Found Apache configuration directory : ${APACHE_CONF_DIR}" )
IF(EXISTS "${APACHE_CONF_DIR}" )
    SET(APACHE_CONF_INSTALL_DIR ${APACHE_CONF_DIR}
        CACHE PATH "Installation directory for apache configuration files")
ELSEIF(EXISTS "${HTTPD_CONF_DIR}")
    SET(APACHE_CONF_INSTALL_DIR ${HTTPD_CONF_DIR}
        CACHE PATH "Installation directory for apache configuration files")
ELSE(EXISTS "${APACHE_CONF_DIR}" )
    SET(APACHE_CONF_INSTALL_DIR "/etc/httpd/conf.d/"
        CACHE PATH "Installation directory for apache configuration files")
ENDIF(EXISTS "${APACHE_CONF_DIR}")


message(STATUS " Found Apache configuration directory : ${APACHE_CONF_INSTALL_DIR}" )
