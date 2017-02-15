#ifndef __VERSION_H__
#define __VERSION_H__

#define PRODUCT_NAME        "Demo XXX"
#define BUILD_DATE          "Date: "##__DATE__##" "##__TIME__##""


#define VERSION_FW_MAJOR    0
#define VERSION_FW_MINOR    1
#define VERSION_FW_REV      2
#define VERSION_FW_BUILD    19

#define VERSION_HW_REV      3
#define VERSION_HW_BUILD    2


#define MAKE_FW_VERSION2(major, minor, rev, build)     #major"."#minor"."#rev"."#build
#define MAKE_FW_VERSION(major, minor, rev, build)      MAKE_FW_VERSION2(major, minor, rev, build)

#define MAKE_HW_VERSION2(rev, build)                   #rev"."#build
#define MAKE_HW_VERSION(rev, build)                    MAKE_HW_VERSION2(rev, build)

#define VERSION_FW          MAKE_FW_VERSION(VERSION_FW_MAJOR, VERSION_FW_MINOR, VERSION_FW_REV, VERSION_FW_BUILD)
#define VERSION_HW          MAKE_HW_VERSION(VERSION_HW_REV, VERSION_HW_BUILD)

#endif /* __VERSION_H__ */

/* vim: set ts=2 sw=2 tw=0 list : */
