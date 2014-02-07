cmake_minimum_required (VERSION 2.6)


set(rsyslog_dir "${SYSCONF_INSTALL_DIR}/rsyslog.d/")
set(logrotate_dir "${SYSCONF_INSTALL_DIR}/logrotate.d/")


IF(RSYSLOG_SUPPORT)
message(STATUS "rsyslog support enabled")

# install syslog scripts
install(FILES "conf/20-log-ugr.conf"
              DESTINATION "${rsyslog_dir}")
ENDIF(RSYSLOG_SUPPORT)


IF(LOGROTATE_SUPPORT)
message(STATUS "logrotate support enabled")

# install syslog scripts
install(FILES "conf/logrotate/ugr-server"
              DESTINATION "${logrotate_dir}")
ENDIF(LOGROTATE_SUPPORT)
