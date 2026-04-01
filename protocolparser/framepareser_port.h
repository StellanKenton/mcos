/************************************************************************************
* @file     : framepareser_port.h
* @brief    : Project port helpers for the stream packet parser.
* @details  : Supplies default timing hooks and parser initialization helpers.
* @author   : GitHub Copilot
* @date     : 2026-04-01
* @version  : V1.0.0
***********************************************************************************/
#ifndef FRM_PSR_PORT_H
#define FRM_PSR_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "framepareser.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef FRM_PSR_PORT_WAIT_PKT_TOUT_MS
#define FRM_PSR_PORT_WAIT_PKT_TOUT_MS    60U
#endif

#ifndef FRM_PSR_PORT_MAX_FMTS
#define FRM_PSR_PORT_MAX_FMTS            4U
#endif

uint32_t frmPsrPortGetTickMs(void);
void frmPsrPortApplyDftCfg(stFrmPsrCfg *cfg);
void frmPsrPortApplyDftRunCfg(stFrmPsrRunCfg *runCfg);
uint32_t frmPsrPortGetFmtCnt(void);
bool frmPsrPortSetFmt(uint32_t idx, const stFrmPsrFmt *fmt);
const stFrmPsrFmt *frmPsrPortGetFmt(uint32_t idx);
eFrmPsrSta frmPsrPortInit(stFrmPsr *psr, stRingBuffer *ringBuf, stFrmPsrCfg *cfg);
eFrmPsrSta frmPsrPortInitFmt(stFrmPsr *psr, stRingBuffer *ringBuf, uint32_t idx, stFrmPsrRunCfg *runCfg);
eFrmPsrSta frmPsrPortSelFmt(stFrmPsr *psr, uint32_t idx);
eFrmPsrSta frmPsrPortMkPkt(uint32_t idx, const uint8_t *payloadBuf, uint16_t payloadLen, uint8_t *pktBuf, uint16_t pktBufSize, uint16_t *pktLen);

#ifdef __cplusplus
}
#endif

#endif  // FRM_PSR_PORT_H
/**************************End of file********************************/
