# @brief cmake define file for apache configuration environment
#
# @author Adrien Devresse


include(DefineInstallationPaths REQUIRED)


SET(APACHE_SITES_DIR "${SYSCONF_INSTALL_DIR}/apache2/sites-available/")
SET(HTTPD_SITES_DIR "${SYSCONF_INSTALL_DIR}/httpd/conf.d/")

message(STATUS " Found Apache configuration directory : ${APACHE_CONF_DIR}" )
IF(EXISTS "${APACHE_SITES_DIR}" )
    SET(APACHE_SITES_INSTALL_DIR ${APACHE_SITES_DIR}
        CACHE PATH "Installation directory for apache configuration files")
ELSEIF(EXISTS "${HTTPD_SITES_DIR}")
    SET(APACHE_SITES_INSTALL_DIR ${HTTPD_SITES_DIR}
        CACHE PATH "Installation directory for apache configuration files")
ELSE(EXISTS "${APACHE_SITES_DIR}" )
    SET(APACHE_SITES_INSTALL_DIR "/etc/httpd/conf.d/"
        CACHE PATH "Installation directory for apache configuration files")
ENDIF(EXISTS "${APACHE_SITES_DIR}")


message(STATUS " Found Apache configuration directory : ${APACHE_SITES_INSTALL_DIR}" )
