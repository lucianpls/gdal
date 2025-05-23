/******************************************************************************
 *
 * Project:  GDAL Core
 * Purpose:  Read metadata from Spot imagery.
 * Author:   Alexander Lisovenko
 * Author:   Dmitry Baryshnikov, polimax@mail.ru
 *
 ******************************************************************************
 * Copyright (c) 2014-2015 NextGIS <info@nextgis.ru>
 *
 * SPDX-License-Identifier: MIT
 ****************************************************************************/

#include "cpl_port.h"
#include "reader_spot.h"

#include <ctime>

#include "cpl_conv.h"
#include "cpl_error.h"
#include "cpl_minixml.h"
#include "cpl_string.h"
#include "cpl_time.h"

#include "gdal_mdreader.h"

/**
 * GDALMDReaderSpot()
 */
GDALMDReaderSpot::GDALMDReaderSpot(const char *pszPath,
                                   char **papszSiblingFiles)
    : GDALMDReaderPleiades(pszPath, papszSiblingFiles)
{
    const std::string osDirName = CPLGetDirnameSafe(pszPath);

    if (m_osIMDSourceFilename.empty())
    {
        std::string osIMDSourceFilename =
            CPLFormFilenameSafe(osDirName.c_str(), "METADATA.DIM", nullptr);

        if (CPLCheckForFile(&osIMDSourceFilename[0], papszSiblingFiles))
        {
            m_osIMDSourceFilename = std::move(osIMDSourceFilename);
        }
        else
        {
            osIMDSourceFilename =
                CPLFormFilenameSafe(osDirName.c_str(), "metadata.dim", nullptr);
            if (CPLCheckForFile(&osIMDSourceFilename[0], papszSiblingFiles))
            {
                m_osIMDSourceFilename = std::move(osIMDSourceFilename);
            }
        }
    }

    // if the file name ended on METADATA.DIM
    // Linux specific
    // example: R2_CAT_091028105025131_1\METADATA.DIM
    if (m_osIMDSourceFilename.empty())
    {
        if (EQUAL(CPLGetFilename(pszPath), "IMAGERY.TIF"))
        {
            std::string osIMDSourceFilename =
                CPLGetPathSafe(pszPath) + "\\METADATA.DIM";

            if (CPLCheckForFile(&osIMDSourceFilename[0], papszSiblingFiles))
            {
                m_osIMDSourceFilename = std::move(osIMDSourceFilename);
            }
            else
            {
                osIMDSourceFilename =
                    CPLGetPathSafe(pszPath) + "\\metadata.dim";
                if (CPLCheckForFile(&osIMDSourceFilename[0], papszSiblingFiles))
                {
                    m_osIMDSourceFilename = std::move(osIMDSourceFilename);
                }
            }
        }
    }

    if (!m_osIMDSourceFilename.empty())
        CPLDebug("MDReaderSpot", "IMD Filename: %s",
                 m_osIMDSourceFilename.c_str());
}

/**
 * ~GDALMDReaderSpot()
 */
GDALMDReaderSpot::~GDALMDReaderSpot()
{
}

/**
 * LoadMetadata()
 */
void GDALMDReaderSpot::LoadMetadata()
{
    if (m_bIsMetadataLoad)
        return;

    if (!m_osIMDSourceFilename.empty())
    {
        CPLXMLNode *psNode = CPLParseXMLFile(m_osIMDSourceFilename);

        if (psNode != nullptr)
        {
            CPLXMLNode *psisdNode = CPLSearchXMLNode(psNode, "=Dimap_Document");

            if (psisdNode != nullptr)
            {
                m_papszIMDMD = ReadXMLToList(psisdNode->psChild, m_papszIMDMD);
            }
            CPLDestroyXMLNode(psNode);
        }
    }

    m_papszDEFAULTMD =
        CSLAddNameValue(m_papszDEFAULTMD, MD_NAME_MDTYPE, "DIMAP");

    m_bIsMetadataLoad = true;

    if (nullptr == m_papszIMDMD)
    {
        return;
    }

    // extract imagery metadata
    int nCounter = -1;
    const char *pszSatId1 = CSLFetchNameValue(
        m_papszIMDMD,
        "Dataset_Sources.Source_Information.Scene_Source.MISSION");
    if (nullptr == pszSatId1)
    {
        nCounter = 1;
        for (int i = 0; i < 5; i++)
        {
            pszSatId1 = CSLFetchNameValue(
                m_papszIMDMD, CPLSPrintf("Dataset_Sources.Source_Information_%"
                                         "d.Scene_Source.MISSION",
                                         nCounter));
            if (nullptr != pszSatId1)
                break;
            nCounter++;
        }
    }

    const char *pszSatId2;
    if (nCounter == -1)
        pszSatId2 = CSLFetchNameValue(
            m_papszIMDMD,
            "Dataset_Sources.Source_Information.Scene_Source.MISSION_INDEX");
    else
        pszSatId2 = CSLFetchNameValue(
            m_papszIMDMD, CPLSPrintf("Dataset_Sources.Source_Information_%d."
                                     "Scene_Source.MISSION_INDEX",
                                     nCounter));

    if (nullptr != pszSatId1 && nullptr != pszSatId2)
    {
        m_papszIMAGERYMD = CSLAddNameValue(
            m_papszIMAGERYMD, MD_NAME_SATELLITE,
            CPLSPrintf("%s %s", CPLStripQuotes(pszSatId1).c_str(),
                       CPLStripQuotes(pszSatId2).c_str()));
    }
    else if (nullptr != pszSatId1 && nullptr == pszSatId2)
    {
        m_papszIMAGERYMD = CSLAddNameValue(m_papszIMAGERYMD, MD_NAME_SATELLITE,
                                           CPLStripQuotes(pszSatId1));
    }
    else if (nullptr == pszSatId1 && nullptr != pszSatId2)
    {
        m_papszIMAGERYMD = CSLAddNameValue(m_papszIMAGERYMD, MD_NAME_SATELLITE,
                                           CPLStripQuotes(pszSatId2));
    }

    const char *pszDate;
    if (nCounter == -1)
        pszDate = CSLFetchNameValue(
            m_papszIMDMD,
            "Dataset_Sources.Source_Information.Scene_Source.IMAGING_DATE");
    else
        pszDate = CSLFetchNameValue(
            m_papszIMDMD, CPLSPrintf("Dataset_Sources.Source_Information_%d."
                                     "Scene_Source.IMAGING_DATE",
                                     nCounter));

    if (nullptr != pszDate)
    {
        const char *pszTime;
        if (nCounter == -1)
            pszTime = CSLFetchNameValue(
                m_papszIMDMD,
                "Dataset_Sources.Source_Information.Scene_Source.IMAGING_TIME");
        else
            pszTime = CSLFetchNameValue(
                m_papszIMDMD, CPLSPrintf("Dataset_Sources.Source_Information_%"
                                         "d.Scene_Source.IMAGING_TIME",
                                         nCounter));

        if (nullptr == pszTime)
            pszTime = "00:00:00.0Z";

        char buffer[80];
        GIntBig timeMid =
            GetAcquisitionTimeFromString(CPLSPrintf("%sT%s", pszDate, pszTime));
        struct tm tmBuf;
        strftime(buffer, 80, MD_DATETIMEFORMAT,
                 CPLUnixTimeToYMDHMS(timeMid, &tmBuf));
        m_papszIMAGERYMD =
            CSLAddNameValue(m_papszIMAGERYMD, MD_NAME_ACQDATETIME, buffer);
    }

    m_papszIMAGERYMD =
        CSLAddNameValue(m_papszIMAGERYMD, MD_NAME_CLOUDCOVER, MD_CLOUDCOVER_NA);
}

/**
 * ReadXMLToList()
 */
char **GDALMDReaderSpot::ReadXMLToList(CPLXMLNode *psNode, char **papszList,
                                       const char *pszName)
{
    if (nullptr == psNode)
        return papszList;

    if (psNode->eType == CXT_Text)
    {
        if (!EQUAL(pszName, ""))
            return AddXMLNameValueToList(papszList, pszName, psNode->pszValue);
    }

    if (psNode->eType == CXT_Element && !EQUAL(psNode->pszValue, "Data_Strip"))
    {
        int nAddIndex = 0;
        bool bReset = false;
        for (CPLXMLNode *psChildNode = psNode->psChild; nullptr != psChildNode;
             psChildNode = psChildNode->psNext)
        {
            if (psChildNode->eType == CXT_Element)
            {
                // check name duplicates
                if (nullptr != psChildNode->psNext)
                {
                    if (bReset)
                    {
                        bReset = false;
                        nAddIndex = 0;
                    }

                    if (EQUAL(psChildNode->pszValue,
                              psChildNode->psNext->pszValue))
                    {
                        nAddIndex++;
                    }
                    else
                    {  // the name changed

                        if (nAddIndex > 0)
                        {
                            bReset = true;
                            nAddIndex++;
                        }
                    }
                }
                else
                {
                    if (nAddIndex > 0)
                    {
                        nAddIndex++;
                    }
                }

                char szName[512];
                if (nAddIndex > 0)
                {
                    CPLsnprintf(szName, 511, "%s_%d", psChildNode->pszValue,
                                nAddIndex);
                }
                else
                {
                    CPLStrlcpy(szName, psChildNode->pszValue, 511);
                }

                char szNameNew[512];
                if (CPLStrnlen(pszName, 511) >
                    0)  // if no prefix just set name to node name
                {
                    CPLsnprintf(szNameNew, 511, "%s.%s", pszName, szName);
                }
                else
                {
                    CPLsnprintf(szNameNew, 511, "%s.%s", psNode->pszValue,
                                szName);
                }

                papszList = ReadXMLToList(psChildNode, papszList, szNameNew);
            }
            else
            {
                // Text nodes should always have name
                if (EQUAL(pszName, ""))
                {
                    papszList =
                        ReadXMLToList(psChildNode, papszList, psNode->pszValue);
                }
                else
                {
                    papszList = ReadXMLToList(psChildNode, papszList, pszName);
                }
            }
        }
    }

    // proceed next only on top level

    if (nullptr != psNode->psNext && EQUAL(pszName, ""))
    {
        papszList = ReadXMLToList(psNode->psNext, papszList, pszName);
    }

    return papszList;
}
