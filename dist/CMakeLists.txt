cmake_minimum_required (VERSION 2.6)


set(rsyslog_dir "${SYSCONF_INSTALL_DIR}/rsyslog.d/")



IF(RSYSLOG_SUPPORT)
message(STATUS "rsyslog support enabled")

# install syslog scripts
install(FILES "conf/20-log-ugr.conf"
              DESTINATION "${rsyslog_dir}")

ELSE(RSYSLOG_SUPPORT)
message(STATUS "rsyslog support disabled")

ENDIF(RSYSLOG_SUPPORT)
