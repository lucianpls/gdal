/******************************************************************************
 *
 * Project:  Oracle Spatial Driver
 * Purpose:  Implementation of OGROCIStatement, which encapsulates the
 *           preparation, execution and fetching from an SQL statement.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#include "ogr_oci.h"
#include "cpl_conv.h"

/************************************************************************/
/*                          OGROCIStatement()                           */
/************************************************************************/

OGROCIStatement::OGROCIStatement(OGROCISession *poSessionIn)

{
    poSession = poSessionIn;
    hStatement = nullptr;
    poDefn = nullptr;

    nRawColumnCount = 0;
    papszCurColumn = nullptr;
    papszCurImage = nullptr;
    panCurColumnInd = nullptr;
    panFieldMap = nullptr;

    pszCommandText = nullptr;
    nAffectedRows = 0;
}

/************************************************************************/
/*                          ~OGROCIStatement()                          */
/************************************************************************/

OGROCIStatement::~OGROCIStatement()

{
    Clean();
}

/************************************************************************/
/*                               Clean()                                */
/************************************************************************/

void OGROCIStatement::Clean()

{
    int i;

    CPLFree(pszCommandText);
    pszCommandText = nullptr;

    if (papszCurColumn != nullptr)
    {
        for (i = 0; papszCurColumn[i] != nullptr; i++)
            CPLFree(papszCurColumn[i]);
    }
    CPLFree(papszCurColumn);
    papszCurColumn = nullptr;

    CPLFree(papszCurImage);
    papszCurImage = nullptr;

    CPLFree(panCurColumnInd);
    panCurColumnInd = nullptr;

    CPLFree(panFieldMap);
    panFieldMap = nullptr;

    if (poDefn != nullptr && poDefn->Dereference() <= 0)
    {
        delete poDefn;
        poDefn = nullptr;
    }

    if (hStatement != nullptr)
    {
        OCIHandleFree(static_cast<dvoid *>(hStatement), (ub4)OCI_HTYPE_STMT);
        hStatement = nullptr;
    }
}

/************************************************************************/
/*                              Prepare()                               */
/************************************************************************/

CPLErr OGROCIStatement::Prepare(const char *pszSQLStatement)

{
    Clean();

    CPLDebug("OCI", "Prepare(%s)", pszSQLStatement);

    pszCommandText = CPLStrdup(pszSQLStatement);

    if (hStatement != nullptr)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Statement already executed once on this OGROCIStatement.");
        return CE_Failure;
    }

    /* -------------------------------------------------------------------- */
    /*      Allocate a statement handle.                                    */
    /* -------------------------------------------------------------------- */
    if (poSession->Failed(
            OCIHandleAlloc(poSession->hEnv,
                           reinterpret_cast<dvoid **>(&hStatement),
                           (ub4)OCI_HTYPE_STMT, 0, nullptr),
            "OCIHandleAlloc(Statement)"))
        return CE_Failure;

    /* -------------------------------------------------------------------- */
    /*      Prepare the statement.                                          */
    /* -------------------------------------------------------------------- */
    if (poSession->Failed(
            OCIStmtPrepare(
                hStatement, poSession->hError,
                reinterpret_cast<text *>(const_cast<char *>(pszSQLStatement)),
                static_cast<unsigned int>(strlen(pszSQLStatement)),
                (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT),
            "OCIStmtPrepare"))
        return CE_Failure;

    return CE_None;
}

/************************************************************************/
/*                             BindObject()                             */
/************************************************************************/

CPLErr OGROCIStatement::BindObject(const char *pszPlaceName, void *pahObjects,
                                   OCIType *hTDO, void **papIndicators)

{
    OCIBind *hBindOrd = nullptr;

    if (poSession->Failed(
            OCIBindByName(
                hStatement, &hBindOrd, poSession->hError,
                reinterpret_cast<text *>(const_cast<char *>(pszPlaceName)),
                (sb4)strlen(pszPlaceName), nullptr, 0, SQLT_NTY, nullptr,
                nullptr, nullptr, (ub4)0, nullptr, (ub4)OCI_DEFAULT),
            "OCIBindByName()"))
        return CE_Failure;

    if (poSession->Failed(
            OCIBindObject(hBindOrd, poSession->hError, hTDO,
                          reinterpret_cast<dvoid **>(pahObjects), nullptr,
                          reinterpret_cast<dvoid **>(papIndicators), nullptr),
            "OCIBindObject()"))
        return CE_Failure;

    return CE_None;
}

/************************************************************************/
/*                             BindScalar()                             */
/************************************************************************/

CPLErr OGROCIStatement::BindScalar(const char *pszPlaceName, void *pData,
                                   int nDataLen, int nSQLType, sb2 *paeInd)

{
    OCIBind *hBindOrd = nullptr;

    if (poSession->Failed(
            OCIBindByName(
                hStatement, &hBindOrd, poSession->hError,
                reinterpret_cast<text *>(const_cast<char *>(pszPlaceName)),
                (sb4)strlen(pszPlaceName), static_cast<dvoid *>(pData),
                (sb4)nDataLen, (ub2)nSQLType, static_cast<dvoid *>(paeInd),
                nullptr, nullptr, 0, nullptr, (ub4)OCI_DEFAULT),
            "OCIBindByName()"))
        return CE_Failure;
    else
        return CE_None;
}

/************************************************************************/
/*                             BindString()                             */
/************************************************************************/

CPLErr OGROCIStatement::BindString(const char *pszPlaceName,
                                   const char *pszData, sb2 *paeInd)

{
    return BindScalar(
        pszPlaceName,
        const_cast<void *>(reinterpret_cast<const void *>(pszData)),
        static_cast<int>(strlen(pszData)) + 1, SQLT_STR, paeInd);
}

/************************************************************************/
/*                              Execute()                               */
/************************************************************************/

CPLErr OGROCIStatement::Execute(const char *pszSQLStatement, int nMode)

{
    /* -------------------------------------------------------------------- */
    /*      Prepare the statement if it is being passed in.                 */
    /* -------------------------------------------------------------------- */
    if (pszSQLStatement != nullptr)
    {
        CPLErr eErr = Prepare(pszSQLStatement);
        if (eErr != CE_None)
            return eErr;
    }

    if (hStatement == nullptr)
    {
        CPLError(
            CE_Failure, CPLE_AppDefined,
            "No prepared statement in call to OGROCIStatement::Execute(NULL)");
        return CE_Failure;
    }

    /* -------------------------------------------------------------------- */
    /*      Determine if this is a SELECT statement.                        */
    /* -------------------------------------------------------------------- */
    ub2 nStmtType;

    if (poSession->Failed(OCIAttrGet(hStatement, OCI_HTYPE_STMT, &nStmtType,
                                     nullptr, OCI_ATTR_STMT_TYPE,
                                     poSession->hError),
                          "OCIAttrGet(ATTR_STMT_TYPE)"))
        return CE_Failure;

    int bSelect = (nStmtType == OCI_STMT_SELECT);

    /* -------------------------------------------------------------------- */
    /*      Work out some details about execution mode.                     */
    /* -------------------------------------------------------------------- */
    if (nMode == -1)
    {
        if (bSelect)
            nMode = OCI_DEFAULT;
        else
            nMode = OCI_COMMIT_ON_SUCCESS;
    }

    /* -------------------------------------------------------------------- */
    /*      Execute the statement.                                          */
    /* -------------------------------------------------------------------- */
    if (poSession->Failed(
            OCIStmtExecute(poSession->hSvcCtx, hStatement, poSession->hError,
                           (ub4)bSelect ? 0 : 1, (ub4)0, (OCISnapshot *)nullptr,
                           (OCISnapshot *)nullptr, nMode),
            pszCommandText))
        return CE_Failure;

    if (!bSelect)
    {
        ub4 row_count;
        if (poSession->Failed(OCIAttrGet(hStatement, OCI_HTYPE_STMT, &row_count,
                                         nullptr, OCI_ATTR_ROW_COUNT,
                                         poSession->hError),
                              "OCIAttrGet(OCI_ATTR_ROW_COUNT)"))
            return CE_Failure;
        nAffectedRows = row_count;

        return CE_None;
    }

    /* -------------------------------------------------------------------- */
    /*      Count the columns.                                              */
    /* -------------------------------------------------------------------- */
    for (nRawColumnCount = 0; true; nRawColumnCount++)
    {
        OCIParam *hParamDesc;

        if (OCIParamGet(hStatement, OCI_HTYPE_STMT, poSession->hError,
                        reinterpret_cast<dvoid **>(&hParamDesc),
                        (ub4)nRawColumnCount + 1) != OCI_SUCCESS)
            break;
    }

    panFieldMap = (int *)CPLCalloc(sizeof(int), nRawColumnCount);

    papszCurColumn = (char **)CPLCalloc(sizeof(char *), nRawColumnCount + 1);
    panCurColumnInd = (sb2 *)CPLCalloc(sizeof(sb2), nRawColumnCount + 1);

    /* ==================================================================== */
    /*      Establish result column definitions, and setup parameter        */
    /*      defines.                                                        */
    /* ==================================================================== */
    poDefn = new OGRFeatureDefn(pszCommandText);
    poDefn->SetGeomType(wkbNone);
    poDefn->Reference();

    for (int iParam = 0; iParam < nRawColumnCount; iParam++)
    {
        OGRFieldDefn oField("", OFTString);
        OCIParam *hParamDesc;
        ub2 nOCIType;
        ub4 nOCILen;

        /* --------------------------------------------------------------------
         */
        /*      Get parameter definition. */
        /* --------------------------------------------------------------------
         */
        if (poSession->Failed(
                OCIParamGet(hStatement, OCI_HTYPE_STMT, poSession->hError,
                            reinterpret_cast<dvoid **>(&hParamDesc),
                            (ub4)iParam + 1),
                "OCIParamGet"))
            return CE_Failure;

        if (poSession->GetParamInfo(hParamDesc, &oField, &nOCIType, &nOCILen) !=
            CE_None)
            return CE_Failure;

        if (oField.GetType() == OFTBinary)
        {
            /* We could probably generalize that, but at least it works in that
             */
            /* use case */
            if (EQUAL(oField.GetNameRef(), "DATA_DEFAULT") &&
                nOCIType == SQLT_LNG)
            {
                oField.SetType(OFTString);
            }
            else
            {
                panFieldMap[iParam] = -1;
                continue;
            }
        }

        poDefn->AddFieldDefn(&oField);
        panFieldMap[iParam] = poDefn->GetFieldCount() - 1;

        /* --------------------------------------------------------------------
         */
        /*      Prepare a binding. */
        /* --------------------------------------------------------------------
         */
        int nBufWidth = 256, nOGRField = panFieldMap[iParam];
        OCIDefine *hDefn = nullptr;

        if (oField.GetWidth() > 0)
            /* extra space needed for the decimal separator the string
            terminator and the negative sign (Tamas Szekeres)*/
            nBufWidth = oField.GetWidth() + 3;
        else if (oField.GetType() == OFTInteger)
            nBufWidth = 22;
        else if (oField.GetType() == OFTReal)
            nBufWidth = 36;
        else if (oField.GetType() == OFTDateTime)
            nBufWidth = 40;
        else if (oField.GetType() == OFTDate)
            nBufWidth = 20;

        papszCurColumn[nOGRField] = (char *)CPLMalloc(nBufWidth + 2);
        CPLAssert(((long)papszCurColumn[nOGRField]) % 2 == 0);

        if (poSession->Failed(
                OCIDefineByPos(
                    hStatement, &hDefn, poSession->hError, iParam + 1,
                    reinterpret_cast<ub1 *>(papszCurColumn[nOGRField]),
                    nBufWidth, SQLT_STR, panCurColumnInd + nOGRField, nullptr,
                    nullptr, OCI_DEFAULT),
                "OCIDefineByPos"))
            return CE_Failure;
    }

    return CE_None;
}

/************************************************************************/
/*                           SimpleFetchRow()                           */
/************************************************************************/

char **OGROCIStatement::SimpleFetchRow()

{
    int nStatus, i;

    if (papszCurImage == nullptr)
    {
        papszCurImage = (char **)CPLCalloc(sizeof(char *), nRawColumnCount + 1);
    }

    nStatus = OCIStmtFetch(hStatement, poSession->hError, 1, OCI_FETCH_NEXT,
                           OCI_DEFAULT);

    if (nStatus == OCI_NO_DATA)
        return nullptr;
    else if (poSession->Failed(nStatus, "OCIStmtFetch"))
        return nullptr;

    for (i = 0; papszCurColumn[i] != nullptr; i++)
    {
        if (panCurColumnInd[i] == OCI_IND_NULL)
            papszCurImage[i] = nullptr;
        else
            papszCurImage[i] = papszCurColumn[i];
    }

    return papszCurImage;
}
