#!/usr/bin/env pytest
###############################################################################
#
# Project:  GDAL/OGR Test Suite
# Purpose:  NAS Reading Driver testing.
# Author:   Even Rouault <even dot rouault at spatialys.com>
#
###############################################################################
# Copyright (c) 2010-2012, Even Rouault <even dot rouault at spatialys.com>
#
# SPDX-License-Identifier: MIT
###############################################################################

import os

import gdaltest
import ogrtest
import pytest

from osgeo import gdal, ogr

# Other test data :
# http://www.lv-bw.de/alkis.info/nas-bsp.html
# http://www.lv-bw.de/lvshop2/Produktinfo/AAA/AAA.html
# http://www.gll.niedersachsen.de/live/live.php?navigation_id=10640&article_id=51644&_psmand=34

pytestmark = pytest.mark.require_driver("NAS")

###############################################################################
# Test reading a NAS file
#


@pytest.mark.skipif(
    not os.path.exists("tmp/cache/nas_testdaten_peine.zip"),
    reason="Test data no longer available",
)
def test_ogr_nas_1():

    gdaltest.download_or_skip(
        "http://www.geodatenzentrum.de/gdz1/abgabe/testdaten/vektor/nas_testdaten_peine.zip",
        "nas_testdaten_peine.zip",
    )

    try:
        os.stat("tmp/cache/BKG_NAS_Peine.xml")
    except OSError:
        try:
            gdaltest.unzip("tmp/cache", "tmp/cache/nas_testdaten_peine.zip")
            try:
                os.stat("tmp/cache/BKG_NAS_Peine.xml")
            except OSError:
                pytest.skip()
        except OSError:
            pytest.skip()

    try:
        os.remove("tmp/cache/BKG_NAS_Peine.gfs")
    except OSError:
        pass

    with gdal.config_option("NAS_GFS_TEMPLATE", ""):
        ds = ogr.Open("tmp/cache/BKG_NAS_Peine.xml")
    assert ds is not None, "could not open dataset"

    assert ds.GetLayerCount() == 40, "did not get expected layer count"

    lyr = ds.GetLayerByName("AX_Wohnplatz")
    feat = lyr.GetNextFeature()
    geom = feat.GetGeometryRef()

    if (
        feat.GetField("name") != "Ziegelei"
        or geom.ExportToWkt() != "POINT (3575300 5805100)"
    ):
        feat.DumpReadable()
        pytest.fail()

    relation_lyr = ds.GetLayerByName("ALKIS_beziehungen")
    feat = relation_lyr.GetNextFeature()
    if (
        feat.GetField("beziehung_von") != "DENIBKG1000001UG"
        or feat.GetField("beziehungsart") != "istTeilVon"
        or feat.GetField("beziehung_zu") != "DENIBKG1000000T6"
    ):
        feat.DumpReadable()
        pytest.fail()

    ds = None


###############################################################################
# Test reading a sample NAS file from PostNAS
#


def test_ogr_nas_2():

    gdaltest.download_or_skip(
        "http://trac.wheregroup.com/PostNAS/browser/trunk/demodaten/lverm_geo_rlp/gid-6.0/gm2566-testdaten-gid60-2008-11-11.xml.zip?format=raw",
        "gm2566-testdaten-gid60-2008-11-11.xml.zip",
    )

    try:
        os.stat("tmp/cache/gm2566-testdaten-gid60-2008-11-11.xml")
    except OSError:
        try:
            gdaltest.unzip(
                "tmp/cache", "tmp/cache/gm2566-testdaten-gid60-2008-11-11.xml.zip"
            )
            try:
                os.stat("tmp/cache/gm2566-testdaten-gid60-2008-11-11.xml")
            except OSError:
                pytest.skip()
        except OSError:
            pytest.skip()

    try:
        os.remove("tmp/cache/gm2566-testdaten-gid60-2008-11-11.gfs")
    except OSError:
        pass

    with gdal.config_option("NAS_GFS_TEMPLATE", ""):
        ds = ogr.Open("tmp/cache/gm2566-testdaten-gid60-2008-11-11.xml")

    assert ds.GetLayerCount() == 84, "did not get expected layer count"

    lyr = ds.GetLayerByName("AX_Flurstueck")

    # Loop until a feature that has a complex geometry including <gml:Arc>
    feat = lyr.GetNextFeature()
    while (
        feat is not None
        and feat.GetField("identifier") != "urn:adv:oid:DERP1234000002Iz"
    ):
        feat = lyr.GetNextFeature()
    assert feat is not None

    # expected_geom = 'POLYGON ((350821.045 5532031.37,350924.309 5532029.513,350938.493 5532026.622,350951.435 5532021.471,350978.7 5532007.18,351026.406 5531971.088,351032.251 5531951.162,351080.623 5531942.67,351154.886 5531963.718,351207.689 5532019.797,351211.063 5532044.067,351203.83 5532074.034,351165.959 5532114.315,351152.85 5532135.774,351141.396 5532140.355,351110.659 5532137.542,351080.17 5532132.742,351002.887 5532120.75,350925.682 5532108.264,350848.556 5532095.285,350771.515 5532081.814,350769.548 5532071.196,350812.194 5532034.716,350821.045 5532031.37))'
    expected_geom = "CURVEPOLYGON (COMPOUNDCURVE ((350821.045 5532031.37,350924.309 5532029.513,350938.493 5532026.622,350951.435 5532021.471,350978.7 5532007.18,351026.406 5531971.088,351032.251 5531951.16199999955),(351032.251 5531951.16199999955,351080.623 5531942.67,351154.886 5531963.718),(351154.886 5531963.718,351207.689 5532019.797),(351207.689 5532019.797,351211.063 5532044.06699999981,351203.83 5532074.034,351165.959 5532114.315,351152.85 5532135.774),(351152.85 5532135.774,351141.396 5532140.355),CIRCULARSTRING (351141.396 5532140.355,351110.659 5532137.542,351080.17 5532132.74199999962),CIRCULARSTRING (351080.17 5532132.74199999962,351002.887 5532120.75,350925.682 5532108.264),CIRCULARSTRING (350925.682 5532108.264,350848.556 5532095.285,350771.515 5532081.814),(350771.515 5532081.814,350769.548 5532071.196,350812.194 5532034.716,350821.045 5532031.37)))"
    ogrtest.check_feature_geometry(feat, expected_geom)

    ds = None


###############################################################################
# Test that we can open and read empty files successfully.
#


def test_ogr_nas_3():

    with gdal.config_option("NAS_GFS_TEMPLATE", ""):
        ds = ogr.Open("data/nas/empty_nas.xml")

    assert ds.GetLayerCount() == 0, "did not get expected layer count"

    ds = None


###############################################################################
# Test that we can read files with wfs:Delete transactions in them properly.
#


def test_ogr_nas_4():

    try:
        os.remove("data/nas/delete_nas.gfs")
    except OSError:
        pass

    with gdal.config_option("NAS_GFS_TEMPLATE", ""):
        ds = ogr.Open("data/nas/delete_nas.xml")

    assert ds.GetLayerCount() == 1, "did not get expected layer count"

    del_lyr = ds.GetLayerByName("Delete")

    assert del_lyr.GetFeatureCount() == 3, "did not get expected number of features"

    del_lyr.ResetReading()
    feat = del_lyr.GetNextFeature()

    assert feat.GetField("context") == "Delete", "did not get expected context"

    assert (
        feat.GetField("typeName") == "AX_Namensnummer"
    ), "did not get expected typeName"

    assert (
        feat.GetField("FeatureId") == "DENW44AL00000HJU20100730T092847Z"
    ), "did not get expected FeatureId"

    del_lyr = None
    ds = None

    try:
        os.remove("data/nas/delete_nas.gfs")
    except OSError:
        pass


###############################################################################
# Test that we can read files with wfsext:Replace transactions properly
#


def test_ogr_nas_5():

    try:
        os.remove("data/nas/replace_nas.gfs")
    except OSError:
        pass

    with gdal.config_option("NAS_GFS_TEMPLATE", ""):
        ds = ogr.Open("data/nas/replace_nas.xml")

    assert ds.GetLayerCount() == 2, "did not get expected layer count"

    # Check the delete operation created for the replace

    del_lyr = ds.GetLayerByName("Delete")

    assert del_lyr.GetFeatureCount() == 1, "did not get expected number of features"

    del_lyr.ResetReading()
    feat = del_lyr.GetNextFeature()

    assert feat.GetField("context") == "Replace", "did not get expected context"

    assert (
        feat.GetField("replacedBy") == "DENW44AL00003IkM20110429T070635Z"
    ), "did not get expected replacedBy"

    assert feat.GetField("safeToIgnore") == "false", "did not get expected safeToIgnore"

    assert feat.GetField("typeName") == "AX_Flurstueck", "did not get expected typeName"

    assert (
        feat.GetField("FeatureId") == "DENW44AL00003IkM20100809T071726Z"
    ), "did not get expected FeatureId"

    del_lyr = None

    # Check also the feature created by the Replace

    lyr = ds.GetLayerByName("AX_Flurstueck")

    assert lyr.GetFeatureCount() == 1, "did not get expected number of features"

    lyr.ResetReading()
    feat = lyr.GetNextFeature()

    assert (
        feat.GetField("gml_id") == "DENW44AL00003IkM20110429T070635Z"
    ), "did not get expected gml_id"

    assert feat.GetField("stelle") == 5212, "did not get expected stelle"

    lyr = None

    ds = None

    try:
        os.remove("data/nas/replace_nas.gfs")
    except OSError:
        pass


###############################################################################
# Test force opening a NAS file


def test_ogr_nas_force_opening(tmp_vsimem):

    filename = str(tmp_vsimem / "test.xml")

    prolog = '<?xml version="1.0" encoding="UTF-8"?>'
    with gdaltest.vsi_open(filename, "wb") as f:
        with open("data/nas/replace_nas.xml", "rb") as fsrc:
            f.write(fsrc.read(len(prolog)) + b" " * (1000 * 1000) + fsrc.read())  # '<'

    with pytest.raises(Exception):
        gdal.OpenEx(filename)

    with gdal.quiet_errors():
        ds = gdal.OpenEx(filename, allowed_drivers=["NAS"])
    assert ds.GetDriver().GetDescription() == "NAS"


###############################################################################
# Test we don't spend too much time parsing documents featuring the billion
# laugh attack


def test_ogr_nas_billion_laugh():

    with gdal.config_option("NAS_GFS_TEMPLATE", ""):
        with gdal.quiet_errors(), pytest.raises(
            Exception, match="File probably corrupted"
        ):
            ogr.Open("data/nas/billionlaugh.xml")
