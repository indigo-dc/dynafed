cmake_minimum_required (VERSION 2.6)


set(rsyslog_dir "${SYSCONF_INSTALL_DIR}/rsyslog.d/")

IF(EXISTS "${rsyslog_dir}" AND IS_DIRECTORY "${rsyslog_dir}")

# install usual scripts

install(FILES "conf/20-log-ugr.conf"
              DESTINATION "${rsyslog_dir}")


ENDIF(EXISTS "${rsyslog_dir}" AND IS_DIRECTORY "${rsyslog_dir}")
