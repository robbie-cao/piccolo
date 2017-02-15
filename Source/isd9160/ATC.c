#include <string.h>
#include <stdint.h>

#include "_types.h"
#include "parser.h"

#include "ISD9160.h"
#include "Log.h"
#include "ATC.h"

#define LOG_TAG         "AT"

#define DEBUG_ATC       1

#if DEBUG_ATC
#define LOGD_ATC        LOGD
#else
#define LOGD_ATC        __LOGD
#endif

#define CMD_STR(str)    "~i~s*AT~s*+~s*"##str##"~s*"

enum {
    AT_CMD_MODE = 0,
    AT_CMD_PM,
    AT_CMD_CONF,
    AT_CMD_SYS,
    AT_CMD_RST,
    AT_CMD_SN,
    AT_CMD_NAME,
    AT_CMD_VOL,

    AT_CMD_TOTAL
};

const Keyword_t atCmdList[] =
{
    { AT_CMD_MODE , CMD_STR("MODE~s*=~s*(~d+)")                                },
    { AT_CMD_PM   , CMD_STR("PM~s*=~s*(~d+)")                                  },
    { AT_CMD_CONF , CMD_STR("CONF~s*=~s*(~d+)~s*,~s*(~d+)~s*,~s*(~d+)")        },
    { AT_CMD_SYS  , CMD_STR("SYS")                                             },
    { AT_CMD_RST  , CMD_STR("RST")                                             },
    { AT_CMD_SN   , CMD_STR("SN~s*=~s*(~d+)~s*,~s*(~w+)")                      },
    { AT_CMD_NAME , CMD_STR("NAME~s*=~s*(~d+)~s*,~s*(~w+)")                    },
    { AT_CMD_VOL  , CMD_STR("VOL~s*=~s*(~d+),~s*(~d+)")                        },

    { 0xFF            , 0                                                          }
};

void ATC_Handler(uint8_t *buf)
{
    ParserMatch_t match;       //  pattern-matching results
    ParserToken_t token[9];    //  pattern-matching tokens
    char      tknBuf[32];
    uint16_t    tmpVal;
    uint16_t    tmpVal2;
    const Keyword_t* kwd;
    Result_t result = RESULT_ERROR;

    for (kwd = atCmdList; kwd->pattern != NULL; kwd++) {
        memset(token, 0, sizeof(token));
        memset(tknBuf, 0, sizeof(tknBuf));
        ParserInitMatch((char *)buf, &match);

        result = ParserMatchPattern(kwd->pattern, (char *)buf, &match, token);

        if (result != RESULT_OK) {
            // Skip if not match
            continue;
        }

        switch (kwd->val) {
            case AT_CMD_MODE:
                LOGD_ATC(LOG_TAG, "MODE\r\n");
                ParserTknToUInt(&token[0], &tmpVal) ;
                LOGD_ATC(LOG_TAG, "Val0 : %d\r\n", tmpVal);
                break;
            case AT_CMD_PM:
                LOGD_ATC(LOG_TAG, "PM\r\n");
                ParserTknToUInt(&token[0], &tmpVal) ;
                LOGD_ATC(LOG_TAG, "Val0 : %d\r\n", tmpVal);
                break;
            case AT_CMD_CONF:
                {
                    uint8_t rw = 0;
                    uint8_t idx = 0;
                    uint16_t val = 0;
                    LOGD_ATC(LOG_TAG, "CONF\r\n");
                    ParserTknToUInt(&token[0], &tmpVal) ;
                    rw = tmpVal;
                    LOGD_ATC(LOG_TAG, "Val0 : %d\r\n", tmpVal);
                    ParserTknToUInt(&token[1], &tmpVal) ;
                    idx = tmpVal;
                    LOGD_ATC(LOG_TAG, "Val1 : %d\r\n", tmpVal);
                    ParserTknToUInt(&token[2], &tmpVal) ;
                    val = tmpVal;
                    LOGD_ATC(LOG_TAG, "Val2 : %d\r\n", tmpVal);
                }
                break;
            case AT_CMD_SYS:
                LOGD_ATC(LOG_TAG, "SYS\r\n");
                break;
            case AT_CMD_RST:
                LOG("RST\r\n");
                DrvSYS_Delay(10000);
                UNLOCKREG();
                DrvSYS_ResetChip();
                // Will never reach here as chip reset
                LOCKREG();
                break;
            case AT_CMD_SN:
                LOGD_ATC(LOG_TAG, "SN\r\n");
                break;
            case AT_CMD_NAME:
                LOGD_ATC(LOG_TAG, "NAME\r\n");
                break;
            case AT_CMD_VOL:
                LOGD_ATC(LOG_TAG, "VOL\r\n");
                break;
            default:
                break;
        }
        // Exit loop if match
        break;
    }
    if (!kwd->pattern) {
        // Not found match pattern
        LOGD_ATC(LOG_TAG, "Cmd Not Support\r\n");
    }
}

/* vim: set ts=4 sw=4 tw=0 list : */
