/*
Copyright (c) 2016, Lawrence Livermore National Security, LLC.
Produced at the Lawrence Livermore National Laboratory
Written by Mark C. Miller, miller86@llnl.gov
LLNL-CODE-707197. All rights reserved.

This file is part of H5Z-ZFP. Please also read the BSD license
https://raw.githubusercontent.com/LLNL/H5Z-ZFP/master/LICENSE
*/

/*
 * Tests for the H5Z_class3_t string-based filter configuration API
 * (set_config / get_config callbacks).
 *
 * Only compiled when H5Z_ZFP_USE_CLASS3 is defined (i.e. when
 * H5ZFP_USE_CLASS3=ON is passed to CMake).
 */

#ifdef H5Z_ZFP_USE_CLASS3

#include <locale.h>

#include "test_common.h"
#include "H5Zzfp_lib.h"
#include "H5Zzfp_plugin.h"
#include "H5Zdevelop.h"
#include "zfp.h"

#define DSIZE    2048
#define FNAME    "test_zfp_str_config.h5"
#define CHUNK    DSIZE

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

/* Create a DCPL with a single ZFP filter appended via string params. */
static hid_t make_dcpl_str(const char *params_str)
{
    hsize_t chunk = CHUNK;
    hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
    if (dcpl < 0) return -1;
    if (H5Pset_chunk(dcpl, 1, &chunk) < 0) { H5Pclose(dcpl); return -1; }
    H5Z_params_t p = H5Z_PARAMS_STR(params_str);
    if (H5Pappend_filter(dcpl, H5Z_FILTER_ZFP, 0, &p) < 0) { H5Pclose(dcpl); return -1; }
    return dcpl;
}

/* Get the ZFP filter's cd_values from a DCPL. Returns cd_nelmts. */
static size_t get_cd(hid_t dcpl, unsigned cd[8])
{
    unsigned flags; size_t n = 8;
    memset(cd, 0, 8 * sizeof(unsigned));
    if (H5Pget_filter_by_id(dcpl, H5Z_FILTER_ZFP, &flags, &n, cd, 0, NULL, NULL) < 0)
        return 0;
    return n;
}

/* Get the reconstructed parameter string for the first filter in the DCPL.
 * Returns the length; caller owns nothing (static buffer). */
static int get_params_str(hid_t dcpl, char *buf, size_t bufsz)
{
    size_t len = bufsz;
    return H5Pget_filter_params_by_idx(dcpl, 0, buf, len, &len);
}

/* -------------------------------------------------------------------------
 * test_set_config: verify that H5Pappend_filter with H5Z_PARAMS_STR
 * produces the same cd_values as H5Pset_zfp_*_cdata for each mode.
 * ---------------------------------------------------------------------- */

static int test_set_config(void)
{
    unsigned got[8], ref[8];
    size_t   ng, nr;

    H5Z_zfp_initialize();

    /* ---- rate ---- */
    {
        double rate = 8.0;
        hid_t dcpl = make_dcpl_str("mode = \"rate\", rate = 8.0");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        ng = get_cd(dcpl, got);
        nr = 8; H5Pset_zfp_rate_cdata(rate, nr, ref);
        assert(ng == nr);
        assert(got[0] == ref[0]);
        assert(got[2] == ref[2]);
        assert(got[3] == ref[3]);
        H5Pclose(dcpl);
    }

    /* ---- precision ---- */
    {
        unsigned prec = 16;
        hid_t dcpl = make_dcpl_str("mode = \"precision\", prec = 16");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        ng = get_cd(dcpl, got);
        nr = 8; H5Pset_zfp_precision_cdata(prec, nr, ref);
        assert(got[0] == ref[0]);
        assert(got[2] == ref[2]);
        H5Pclose(dcpl);
    }

    /* ---- accuracy ---- */
    {
        double acc = 0.001;
        hid_t dcpl = make_dcpl_str("mode = \"accuracy\", acc = 0.001");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        ng = get_cd(dcpl, got);
        nr = 8; H5Pset_zfp_accuracy_cdata(acc, nr, ref);
        assert(got[0] == ref[0]);
        assert(got[2] == ref[2]);
        assert(got[3] == ref[3]);
        H5Pclose(dcpl);
    }

    /* ---- expert ---- */
    {
        hid_t dcpl = make_dcpl_str(
            "mode = \"expert\", minbits = 1, maxbits = 4171, maxprec = 64, minexp = -1074");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        ng = get_cd(dcpl, got);
        nr = 8; H5Pset_zfp_expert_cdata(1, 4171, 64, -1074, nr, ref);
        assert(got[0] == ref[0]);
        assert(got[2] == ref[2]);
        assert(got[3] == ref[3]);
        assert(got[4] == ref[4]);
        assert(got[5] == ref[5]);
        H5Pclose(dcpl);
    }

    /* ---- reversible ---- */
    {
        hid_t dcpl = make_dcpl_str("mode = \"reversible\"");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        ng = get_cd(dcpl, got);
        nr = 8; H5Pset_zfp_reversible_cdata(nr, ref);
        assert(got[0] == ref[0]);
        H5Pclose(dcpl);
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * test_mode_inference: mode inferred from key presence (no explicit mode=)
 * ---------------------------------------------------------------------- */

static int test_mode_inference(void)
{
    unsigned got[8];
    size_t   nr;

    H5Z_zfp_initialize();

    /* infer rate */
    {
        double rate = 4.0;
        hid_t dcpl = make_dcpl_str("rate = 4.0");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        unsigned ref[8]; nr = 8; H5Pset_zfp_rate_cdata(rate, nr, ref);
        assert(got[0] == H5Z_ZFP_MODE_RATE);
        assert(got[2] == ref[2]);
        assert(got[3] == ref[3]);
        H5Pclose(dcpl);
    }

    /* infer precision via alias "prec" */
    {
        hid_t dcpl = make_dcpl_str("prec = 20");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        assert(got[0] == H5Z_ZFP_MODE_PRECISION);
        assert(got[2] == 20);
        H5Pclose(dcpl);
    }

    /* infer precision via alias "precision" */
    {
        hid_t dcpl = make_dcpl_str("precision = 24");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        assert(got[0] == H5Z_ZFP_MODE_PRECISION);
        assert(got[2] == 24);
        H5Pclose(dcpl);
    }

    /* infer accuracy via alias "acc" */
    {
        hid_t dcpl = make_dcpl_str("acc = 0.01");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        assert(got[0] == H5Z_ZFP_MODE_ACCURACY);
        H5Pclose(dcpl);
    }

    /* infer accuracy via alias "accuracy" */
    {
        hid_t dcpl = make_dcpl_str("accuracy = 0.01");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        assert(got[0] == H5Z_ZFP_MODE_ACCURACY);
        H5Pclose(dcpl);
    }

    /* infer expert from "minbits" */
    {
        hid_t dcpl = make_dcpl_str("minbits=0, maxbits=128, maxprec=32, minexp=-1074");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        assert(got[0] == H5Z_ZFP_MODE_EXPERT);
        H5Pclose(dcpl);
    }

    /* infer reversible from "reversible = true" */
    {
        hid_t dcpl = make_dcpl_str("reversible = true");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        assert(got[0] == H5Z_ZFP_MODE_REVERSIBLE);
        H5Pclose(dcpl);
    }

    /* empty string → reversible default */
    {
        hid_t dcpl = make_dcpl_str("");
        if (dcpl < 0) SET_ERROR(H5Pappend_filter);
        get_cd(dcpl, got);
        assert(got[0] == H5Z_ZFP_MODE_REVERSIBLE);
        H5Pclose(dcpl);
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * test_full_roundtrip: the core contract —
 *   set_config(string) → cd_values1
 *   get_config(cd_values1) → reconstructed_string
 *   set_config(reconstructed_string) → cd_values2
 *   assert cd_values1 == cd_values2
 *
 * This verifies that get_config output is a lossless set_config input.
 * ---------------------------------------------------------------------- */

static int test_full_roundtrip(void)
{
    char buf[256];
    unsigned cd1[8], cd2[8];
    size_t   n1, n2;

    H5Z_zfp_initialize();

    /* Helper: given an initial string, run the round-trip and compare. */
#define CHECK_ROUNDTRIP(str) \
    do { \
        hid_t dcpl1 = make_dcpl_str(str); \
        if (dcpl1 < 0) SET_ERROR(H5Pappend_filter); \
        n1 = get_cd(dcpl1, cd1); \
        if (get_params_str(dcpl1, buf, sizeof(buf)) < 0) \
            SET_ERROR(H5Pget_filter_params_by_idx); \
        H5Pclose(dcpl1); \
        /* Re-feed the reconstructed string */ \
        hid_t dcpl2 = make_dcpl_str(buf); \
        if (dcpl2 < 0) { \
            fprintf(stderr, "re-feed failed for: %s\nreconstructed: %s\n", str, buf); \
            SET_ERROR(H5Pappend_filter); \
        } \
        n2 = get_cd(dcpl2, cd2); \
        H5Pclose(dcpl2); \
        assert(n1 == n2); \
        for (size_t _i = 0; _i < n1; _i++) assert(cd1[_i] == cd2[_i]); \
    } while(0)

    CHECK_ROUNDTRIP("rate = 8.0");
    CHECK_ROUNDTRIP("rate = 3.14159265358979");
    CHECK_ROUNDTRIP("precision = 16");
    CHECK_ROUNDTRIP("precision = 32");
    CHECK_ROUNDTRIP("precision = 0");  /* 0 = full precision, per ZFP */
    CHECK_ROUNDTRIP("acc = 0.001");
    CHECK_ROUNDTRIP("acc = 1e-10");
    CHECK_ROUNDTRIP("minbits = 1, maxbits = 4171, maxprec = 64, minexp = -1074");
    CHECK_ROUNDTRIP("minbits = 0, maxbits = 64, maxprec = 32, minexp = -20");
    /* ZFP_MIN_EXP-1 is the sentinel encoding reversible mode in expert params */
    CHECK_ROUNDTRIP("minbits = 0, maxbits = 4171, maxprec = 64, minexp = -1075");
    CHECK_ROUNDTRIP("mode = \"reversible\"");

#undef CHECK_ROUNDTRIP

    return 0;
}

/* -------------------------------------------------------------------------
 * test_expert_introspection: verify that get_config uses
 * zfp_stream_compression_mode() to detect high-level modes when
 * H5Z_ZFP_MODE_EXPERT cd_values happen to match precision or reversible.
 * Only meaningful on ZFP >= 0x0550 (when the API was added).
 * ---------------------------------------------------------------------- */

#if ZFP_VERSION_NO >= 0x0550
static int test_expert_introspection(void)
{
    hsize_t  chunk = CHUNK;
    char     buf[256];
    hid_t    dcpl;
    unsigned cd[6];
    int64_t  ival;

    H5Z_zfp_initialize();

    /* ---- precision=16 expressed as expert cd_values ---- *
     * zfp_stream_set_precision(16) sets:
     *   minbits=0, maxbits=ZFP_MAX_BITS, maxprec=16, minexp=ZFP_MIN_EXP  */
    {
        cd[0] = (unsigned)H5Z_ZFP_MODE_EXPERT;
        cd[1] = 0;
        cd[2] = 0;                        /* minbits */
        cd[3] = (unsigned)ZFP_MAX_BITS;   /* maxbits */
        cd[4] = 16;                       /* maxprec = precision */
        cd[5] = (unsigned)ZFP_MIN_EXP;    /* minexp */

        if (0 > (dcpl = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate);
        if (0 > H5Pset_chunk(dcpl, 1, &chunk)) { H5Pclose(dcpl); SET_ERROR(H5Pset_chunk); }
        if (0 > H5Pset_filter(dcpl, H5Z_FILTER_ZFP, H5Z_FLAG_MANDATORY, 6, cd))
            { H5Pclose(dcpl); SET_ERROR(H5Pset_filter); }
        if (0 > get_params_str(dcpl, buf, sizeof(buf)))
            { H5Pclose(dcpl); SET_ERROR(H5Pget_filter_params_by_idx); }
        H5Pclose(dcpl);

        /* Must reconstruct as precision mode, not raw expert params */
        assert(H5Zconfig_has_key(buf, "prec") > 0 ||
               H5Zconfig_has_key(buf, "precision") > 0);
        if (H5Zconfig_has_key(buf, "prec") > 0) {
            assert(H5Zconfig_get_int(buf, "prec", &ival) > 0 && ival == 16);
        } else {
            assert(H5Zconfig_get_int(buf, "precision", &ival) > 0 && ival == 16);
        }
        /* Must NOT contain raw expert keys */
        assert(H5Zconfig_has_key(buf, "minbits") <= 0);
    }

    /* ---- reversible expressed as expert cd_values ---- *
     * zfp_stream_set_reversible() sets minexp = ZFP_MIN_EXP - 1 (sentinel). */
    {
        cd[0] = (unsigned)H5Z_ZFP_MODE_EXPERT;
        cd[1] = 0;
        cd[2] = 0;                            /* minbits */
        cd[3] = (unsigned)ZFP_MAX_BITS;       /* maxbits */
        cd[4] = (unsigned)ZFP_MAX_PREC;       /* maxprec */
        cd[5] = (unsigned)(ZFP_MIN_EXP - 1);  /* reversible sentinel */

        if (0 > (dcpl = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate);
        if (0 > H5Pset_chunk(dcpl, 1, &chunk)) { H5Pclose(dcpl); SET_ERROR(H5Pset_chunk); }
        if (0 > H5Pset_filter(dcpl, H5Z_FILTER_ZFP, H5Z_FLAG_MANDATORY, 6, cd))
            { H5Pclose(dcpl); SET_ERROR(H5Pset_filter); }
        if (0 > get_params_str(dcpl, buf, sizeof(buf)))
            { H5Pclose(dcpl); SET_ERROR(H5Pget_filter_params_by_idx); }
        H5Pclose(dcpl);

        /* Must reconstruct as reversible, not raw expert params */
        {
            hbool_t rev = 0;
            htri_t  has_rev = H5Zconfig_get_bool(buf, "reversible", &rev);
            if (has_rev > 0) {
                assert(rev == 1);
            } else {
                char ms[32] = ""; size_t msz = sizeof(ms);
                assert(H5Zconfig_get_str(buf, "mode", ms, &msz) > 0);
                assert(strcmp(ms, "reversible") == 0);
            }
        }
        assert(H5Zconfig_has_key(buf, "minbits") <= 0);
    }

    return 0;
}
#endif /* ZFP_VERSION_NO >= 0x0550 */

/* -------------------------------------------------------------------------
 * test_e2e: write datasets with string API, read back and check accuracy.
 * Covers all five modes.  File is kept for test_get_config_from_file.
 * ---------------------------------------------------------------------- */

static int test_e2e(void)
{
    hid_t  fid = -1, sid = -1, dcpl = -1, dsid = -1;
    hsize_t dims = DSIZE, chunk = CHUNK;
    double *wbuf = NULL, *rbuf = NULL;
    int    i, ndiffs;

    H5Z_zfp_initialize();

    if (0 > (sid = H5Screate_simple(1, &dims, NULL))) SET_ERROR(H5Screate_simple);
    if (0 > (fid = H5Fcreate(FNAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)))
        SET_ERROR(H5Fcreate);

    gen_data(DSIZE, 0.01, 10, (void **)&wbuf, TYPDBL);
    rbuf = (double *) malloc(DSIZE * sizeof(double));
    if (!rbuf) SET_ERROR(malloc);

#define WRITE_DS(name, params_str) \
    do { \
        H5Z_params_t _p = H5Z_PARAMS_STR(params_str); \
        if (0 > (dcpl = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate); \
        if (0 > H5Pset_chunk(dcpl, 1, &chunk)) SET_ERROR(H5Pset_chunk); \
        if (0 > H5Pappend_filter(dcpl, H5Z_FILTER_ZFP, 0, &_p)) SET_ERROR(H5Pappend_filter); \
        if (0 > (dsid = H5Dcreate(fid, name, H5T_NATIVE_DOUBLE, sid, \
                                  H5P_DEFAULT, dcpl, H5P_DEFAULT))) SET_ERROR(H5Dcreate); \
        if (0 > H5Dwrite(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, wbuf)) \
            SET_ERROR(H5Dwrite); \
        if (0 > H5Dclose(dsid)) SET_ERROR(H5Dclose); \
        if (0 > H5Pclose(dcpl))  SET_ERROR(H5Pclose); \
    } while(0)

    WRITE_DS("rate8",       "rate = 8.0");
    WRITE_DS("prec16",      "precision = 16");
    WRITE_DS("acc001",      "acc = 0.001");
    WRITE_DS("expert",      "minbits = 0, maxbits = 4171, maxprec = 64, minexp = -1074");
    WRITE_DS("reversible",  "mode = \"reversible\"");

#undef WRITE_DS

    if (0 > H5Fclose(fid)) SET_ERROR(H5Fclose);

    /* ---- re-open and verify ---- */
    if (0 > (fid = H5Fopen(FNAME, H5F_ACC_RDONLY, H5P_DEFAULT))) SET_ERROR(H5Fopen);

#define READ_CHECK(name, max_abserr, exact) \
    do { \
        if (0 > (dsid = H5Dopen(fid, name, H5P_DEFAULT))) SET_ERROR(H5Dopen); \
        if (0 > H5Dread(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, rbuf)) \
            SET_ERROR(H5Dread); \
        H5Dclose(dsid); \
        for (i = 0, ndiffs = 0; i < DSIZE; i++) { \
            if (exact) { if (rbuf[i] != wbuf[i]) ndiffs++; } \
            else { if (fabs(rbuf[i] - wbuf[i]) > (max_abserr)) ndiffs++; } \
        } \
        assert(ndiffs == 0); \
    } while(0)

    READ_CHECK("rate8",      0.5,    0);  /* lossy; smooth signal well within 0.5 */
    READ_CHECK("prec16",     0.01,   0);  /* 16-bit precision; small relative error */
    READ_CHECK("acc001",     0.002,  0);  /* accuracy=0.001 → max error < 0.002 */
    READ_CHECK("expert",     0.5,    0);  /* default-like expert params */
    READ_CHECK("reversible", 0.0,    1);  /* exact */

#undef READ_CHECK

    if (0 > H5Fclose(fid)) SET_ERROR(H5Fclose);
    if (0 > H5Sclose(sid))  SET_ERROR(H5Sclose);

    free(wbuf);
    free(rbuf);
    return 0;
}

/* Typed check helpers used by CHECK_DS in test_get_config_from_file */
static int _check_double(const char *s, const char *k, double exp) {
    double v = 0;
    return H5Zconfig_get_double(s, k, &v) > 0 && v == exp;
}
static int _check_int64_t(const char *s, const char *k, int64_t exp) {
    int64_t v = 0;
    return H5Zconfig_get_int(s, k, &v) > 0 && v == exp;
}

/* -------------------------------------------------------------------------
 * test_get_config_from_file: reopen the file written by test_e2e.
 * For each dataset, call H5Pget_filter_params_by_idx on the stored
 * (ZFP-header) cd_values, verify the exact reconstructed numeric values,
 * then re-feed the string into H5Pappend_filter and write a second dataset
 * that must read back identically to the original.
 * ---------------------------------------------------------------------- */

static int test_get_config_from_file(void)
{
    hid_t   fid = -1, fid2 = -1, sid = -1, dsid = -1, dcpl = -1, dcpl2 = -1;
    char    buf[256], fname2[] = "test_zfp_str_config2.h5";
    size_t  len;
    hsize_t dims = DSIZE, chunk = CHUNK;
    double *orig = NULL, *r1 = NULL, *r2 = NULL;
    int     i, ndiffs;

    gen_data(DSIZE, 0.01, 10, (void **)&orig, TYPDBL);
    r1  = (double *) malloc(DSIZE * sizeof(double));
    r2  = (double *) malloc(DSIZE * sizeof(double));
    if (!r1 || !r2) SET_ERROR(malloc);

    if (0 > (sid  = H5Screate_simple(1, &dims, NULL))) SET_ERROR(H5Screate_simple);
    if (0 > (fid  = H5Fopen(FNAME, H5F_ACC_RDONLY, H5P_DEFAULT))) SET_ERROR(H5Fopen);
    if (0 > (fid2 = H5Fcreate(fname2, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)))
        SET_ERROR(H5Fcreate);

    /* For each dataset: verify reconstructed string values, re-apply, cross-compare. */
#define CHECK_DS(dsname, key, getter, expected_val, tol) \
    do { \
        /* --- read original data --- */ \
        if (0 > (dsid = H5Dopen(fid, dsname, H5P_DEFAULT))) SET_ERROR(H5Dopen); \
        if (0 > H5Dread(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, r1)) \
            SET_ERROR(H5Dread); \
        if (0 > (dcpl = H5Dget_create_plist(dsid))) SET_ERROR(H5Dget_create_plist); \
        H5Dclose(dsid); \
        /* --- size-query then populate --- */ \
        len = 0; \
        if (0 > H5Pget_filter_params_by_idx(dcpl, 0, NULL, 0, &len)) \
            SET_ERROR(H5Pget_filter_params_by_idx); \
        assert(len > 0 && len < sizeof(buf)); \
        len = sizeof(buf); \
        if (0 > H5Pget_filter_params_by_idx(dcpl, 0, buf, len, &len)) \
            SET_ERROR(H5Pget_filter_params_by_idx); \
        H5Pclose(dcpl); \
        /* --- verify exact reconstructed value --- */ \
        assert(H5Zconfig_has_key(buf, key) > 0); \
        assert(_check_##getter(buf, key, expected_val)); \
        /* --- re-apply string to a new DCPL --- */ \
        if (0 > (dcpl2 = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate); \
        if (0 > H5Pset_chunk(dcpl2, 1, &chunk)) SET_ERROR(H5Pset_chunk); \
        { H5Z_params_t _p = H5Z_PARAMS_STR(buf); \
          if (0 > H5Pappend_filter(dcpl2, H5Z_FILTER_ZFP, 0, &_p)) SET_ERROR(H5Pappend_filter); } \
        if (0 > (dsid = H5Dcreate(fid2, dsname, H5T_NATIVE_DOUBLE, sid, \
                                  H5P_DEFAULT, dcpl2, H5P_DEFAULT))) SET_ERROR(H5Dcreate); \
        if (0 > H5Dwrite(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, orig)) \
            SET_ERROR(H5Dwrite); \
        H5Dclose(dsid); H5Pclose(dcpl2); \
    } while(0)

    CHECK_DS("rate8",  "rate",    double,   8.0,    0.0);
    CHECK_DS("acc001", "acc",     double,   0.001,  0.0);
    CHECK_DS("prec16", "prec",    int64_t,  16,     0);

    /* Expert: check all four keys */
    {
        if (0 > (dsid = H5Dopen(fid, "expert", H5P_DEFAULT))) SET_ERROR(H5Dopen);
        if (0 > H5Dread(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, r1))
            SET_ERROR(H5Dread);
        if (0 > (dcpl = H5Dget_create_plist(dsid))) SET_ERROR(H5Dget_create_plist);
        H5Dclose(dsid);
        len = sizeof(buf);
        if (0 > H5Pget_filter_params_by_idx(dcpl, 0, buf, len, &len))
            SET_ERROR(H5Pget_filter_params_by_idx);
        H5Pclose(dcpl);
        int64_t mb = 0, xb = 0, mp = 0, me = 0;
        assert(H5Zconfig_get_int(buf, "minbits", &mb) > 0 && mb == 0);
        assert(H5Zconfig_get_int(buf, "maxbits", &xb) > 0 && xb == 4171);
        assert(H5Zconfig_get_int(buf, "maxprec", &mp) > 0 && mp == 64);
        assert(H5Zconfig_get_int(buf, "minexp",  &me) > 0 && me == -1074);

        if (0 > (dcpl2 = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate);
        if (0 > H5Pset_chunk(dcpl2, 1, &chunk)) SET_ERROR(H5Pset_chunk);
        H5Z_params_t _p = H5Z_PARAMS_STR(buf);
        if (0 > H5Pappend_filter(dcpl2, H5Z_FILTER_ZFP, 0, &_p)) SET_ERROR(H5Pappend_filter);
        if (0 > (dsid = H5Dcreate(fid2, "expert", H5T_NATIVE_DOUBLE, sid,
                                  H5P_DEFAULT, dcpl2, H5P_DEFAULT))) SET_ERROR(H5Dcreate);
        if (0 > H5Dwrite(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, orig))
            SET_ERROR(H5Dwrite);
        H5Dclose(dsid); H5Pclose(dcpl2);
    }

    /* Reversible: verify the reconstructed string can be re-fed */
    {
        if (0 > (dsid = H5Dopen(fid, "reversible", H5P_DEFAULT))) SET_ERROR(H5Dopen);
        if (0 > H5Dread(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, r1))
            SET_ERROR(H5Dread);
        if (0 > (dcpl = H5Dget_create_plist(dsid))) SET_ERROR(H5Dget_create_plist);
        H5Dclose(dsid);
        len = sizeof(buf);
        if (0 > H5Pget_filter_params_by_idx(dcpl, 0, buf, len, &len))
            SET_ERROR(H5Pget_filter_params_by_idx);
        H5Pclose(dcpl);
        /* reversible reconstructs as "reversible = true" or "mode = \"reversible\"" */
        hbool_t rev = 0;
        htri_t has_rev = H5Zconfig_get_bool(buf, "reversible", &rev);
        if (has_rev > 0) { assert(rev == 1); }
        else {
            char ms[32] = ""; size_t msz = sizeof(ms);
            assert(H5Zconfig_get_str(buf, "mode", ms, &msz) > 0);
            assert(strcmp(ms, "reversible") == 0);
        }
        if (0 > (dcpl2 = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate);
        if (0 > H5Pset_chunk(dcpl2, 1, &chunk)) SET_ERROR(H5Pset_chunk);
        H5Z_params_t _p = H5Z_PARAMS_STR(buf);
        if (0 > H5Pappend_filter(dcpl2, H5Z_FILTER_ZFP, 0, &_p)) SET_ERROR(H5Pappend_filter);
        if (0 > (dsid = H5Dcreate(fid2, "reversible", H5T_NATIVE_DOUBLE, sid,
                                  H5P_DEFAULT, dcpl2, H5P_DEFAULT))) SET_ERROR(H5Dcreate);
        if (0 > H5Dwrite(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, orig))
            SET_ERROR(H5Dwrite);
        H5Dclose(dsid); H5Pclose(dcpl2);
    }

    if (0 > H5Fclose(fid))  SET_ERROR(H5Fclose);
    if (0 > H5Fclose(fid2)) SET_ERROR(H5Fclose);

    /* Now reopen fid2 and compare every dataset against r1 (from fid) */
    if (0 > (fid  = H5Fopen(FNAME,  H5F_ACC_RDONLY, H5P_DEFAULT))) SET_ERROR(H5Fopen);
    if (0 > (fid2 = H5Fopen(fname2, H5F_ACC_RDONLY, H5P_DEFAULT))) SET_ERROR(H5Fopen);

    static const char *dsnames[] = {"rate8","prec16","acc001","expert","reversible"};
    /* reversible must be exact; others may differ slightly due to ZFP header precision */
    static const double tols[]   = {1e-6, 1e-6, 1e-6, 1e-6, 0.0};
    for (int d = 0; d < 5; d++) {
        if (0 > (dsid = H5Dopen(fid,  dsnames[d], H5P_DEFAULT))) SET_ERROR(H5Dopen);
        if (0 > H5Dread(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, r1))
            SET_ERROR(H5Dread);
        H5Dclose(dsid);
        if (0 > (dsid = H5Dopen(fid2, dsnames[d], H5P_DEFAULT))) SET_ERROR(H5Dopen);
        if (0 > H5Dread(dsid, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, r2))
            SET_ERROR(H5Dread);
        H5Dclose(dsid);
        for (i = 0, ndiffs = 0; i < DSIZE; i++) {
            if (tols[d] == 0.0) { if (r1[i] != r2[i]) ndiffs++; }
            else { if (fabs(r1[i] - r2[i]) > tols[d]) ndiffs++; }
        }
        assert(ndiffs == 0);
    }

    H5Fclose(fid);
    H5Fclose(fid2);
    H5Sclose(sid);

    /* cleanup second file */
    remove(fname2);

    free(orig); free(r1); free(r2);

#undef CHECK_DS
    return 0;
}

/* -------------------------------------------------------------------------
 * test_errors: bad mode string, missing required keys
 * ---------------------------------------------------------------------- */

static int test_errors(void)
{
    hsize_t chunk = CHUNK;
    hid_t dcpl;
    herr_t ret;

    H5Eset_auto(H5E_DEFAULT, NULL, NULL);
    H5Z_zfp_initialize();

#define EXPECT_FAIL(str) \
    do { \
        if (0 > (dcpl = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate); \
        if (0 > H5Pset_chunk(dcpl, 1, &chunk)) SET_ERROR(H5Pset_chunk); \
        { H5Z_params_t _p = H5Z_PARAMS_STR(str); \
          ret = H5Pappend_filter(dcpl, H5Z_FILTER_ZFP, 0, &_p); } \
        assert(ret < 0); \
        H5Pclose(dcpl); H5Eclear(H5E_DEFAULT); \
    } while(0)

    EXPECT_FAIL("mode = \"bogus\"");          /* unknown mode string */
    EXPECT_FAIL("mode = \"rate\"");           /* rate declared, value missing */
    EXPECT_FAIL("mode = \"precision\"");      /* prec declared, value missing */
    EXPECT_FAIL("mode = \"accuracy\"");       /* acc declared, value missing */
    EXPECT_FAIL("mode = \"expert\"");         /* expert declared, all values missing */
    EXPECT_FAIL("minbits = 0, maxprec = 64, minexp = -1074"); /* expert inferred, maxbits missing */
    EXPECT_FAIL("unknown_key = 42");          /* no recognisable mode-determining keys */

    /* ambiguous: multiple mode-determining keys in inferred mode */
    EXPECT_FAIL("rate = 3.5, precision = 16");
    EXPECT_FAIL("acc = 0.001, minbits = 0, maxbits = 64, maxprec = 32, minexp = -20");

    /* explicit mode= with keys from another mode */
    EXPECT_FAIL("mode = \"rate\", precision = 16");
    EXPECT_FAIL("mode = \"precision\", acc = 0.001");
    EXPECT_FAIL("mode = \"accuracy\", rate = 3.5");
    EXPECT_FAIL("mode = \"reversible\", rate = 3.5");
    EXPECT_FAIL("mode = \"expert\", reversible = true");

    /* out-of-range parameter values */
    EXPECT_FAIL("rate = -1.0");               /* negative rate */
    EXPECT_FAIL("precision = -1");            /* below minimum (0) */
    EXPECT_FAIL("precision = 65");            /* above ZFP_MAX_PREC (64) */
    EXPECT_FAIL("accuracy = -0.001");         /* negative accuracy */
    EXPECT_FAIL("minbits = 100, maxbits = 50, maxprec = 64, minexp = -1074"); /* minbits > maxbits */
    EXPECT_FAIL("minbits = 0, maxbits = 64, maxprec = 32, minexp = -1076");   /* below ZFP_MIN_EXP-1 */

    /* reversible = false with no other mode key → no mode selected */
    EXPECT_FAIL("reversible = false");

#undef EXPECT_FAIL

    /* H5Pget_filter_params_by_idx with a buffer too small must fail safely */
    {
        hid_t  dcpl_t = make_dcpl_str("mode = \"rate\", rate = 8.0");
        if (dcpl_t >= 0) {
            char   small_buf[5] = {0};
            size_t len = sizeof(small_buf);
            H5E_BEGIN_TRY {
                ret = H5Pget_filter_params_by_idx(dcpl_t, 0, small_buf, len, &len);
            } H5E_END_TRY;
            assert(ret < 0);
            H5Pclose(dcpl_t);
            H5Eclear(H5E_DEFAULT);
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------
 * test_locale_float: verify that get_config emits '.' not ',' for float
 * values even when LC_NUMERIC is a comma-decimal locale (e.g. de_DE).
 * Skips silently if the locale is not installed on this system.
 * ---------------------------------------------------------------------- */

static int test_locale_float(void)
{
    char buf[256];
    hid_t dcpl;

    H5Z_zfp_initialize();

    if (!setlocale(LC_NUMERIC, "de_DE.UTF-8") && !setlocale(LC_NUMERIC, "de_DE"))
        return 0;  /* locale not available — skip */

    /* rate: float output must round-trip through TOML correctly */
    {
        double v = 0;
        dcpl = make_dcpl_str("rate = 3.5");
        assert(dcpl >= 0);
        assert(get_params_str(dcpl, buf, sizeof(buf)) >= 0);
        H5Pclose(dcpl);
        assert(H5Zconfig_get_double(buf, "rate", &v) > 0);
        assert(v == 3.5);
    }

    /* accuracy: same check */
    {
        double v = 0;
        dcpl = make_dcpl_str("acc = 1e-3");
        assert(dcpl >= 0);
        assert(get_params_str(dcpl, buf, sizeof(buf)) >= 0);
        H5Pclose(dcpl);
        assert(H5Zconfig_get_double(buf, "acc", &v) > 0);
        assert(v == 1e-3);
    }

    setlocale(LC_NUMERIC, "C");
    return 0;
}

/* -------------------------------------------------------------------------
 * test_conflict_matrix: exhaustive check of all 20 cross-mode conflict
 * combinations (explicit mode= with keys from a different mode).
 * ---------------------------------------------------------------------- */

static int test_conflict_matrix(void)
{
    hsize_t chunk = CHUNK;
    hid_t dcpl;
    herr_t ret;

    H5Eset_auto(H5E_DEFAULT, NULL, NULL);
    H5Z_zfp_initialize();

#define EXPECT_FAIL(str) \
    do { \
        if (0 > (dcpl = H5Pcreate(H5P_DATASET_CREATE))) SET_ERROR(H5Pcreate); \
        if (0 > H5Pset_chunk(dcpl, 1, &chunk)) SET_ERROR(H5Pset_chunk); \
        { H5Z_params_t _p = H5Z_PARAMS_STR(str); \
          ret = H5Pappend_filter(dcpl, H5Z_FILTER_ZFP, 0, &_p); } \
        assert(ret < 0); \
        H5Pclose(dcpl); H5Eclear(H5E_DEFAULT); \
    } while(0)

    /* rate mode + alien keys */
    EXPECT_FAIL("mode = \"rate\", rate = 4.0, prec = 16");
    EXPECT_FAIL("mode = \"rate\", rate = 4.0, acc = 0.001");
    EXPECT_FAIL("mode = \"rate\", rate = 4.0, minbits = 0, maxbits = 64, maxprec = 32, minexp = -20");
    EXPECT_FAIL("mode = \"rate\", rate = 4.0, reversible = true");

    /* precision mode + alien keys */
    EXPECT_FAIL("mode = \"precision\", prec = 16, rate = 4.0");
    EXPECT_FAIL("mode = \"precision\", prec = 16, acc = 0.001");
    EXPECT_FAIL("mode = \"precision\", prec = 16, minbits = 0, maxbits = 64, maxprec = 32, minexp = -20");
    EXPECT_FAIL("mode = \"precision\", prec = 16, reversible = true");

    /* accuracy mode + alien keys */
    EXPECT_FAIL("mode = \"accuracy\", acc = 0.001, rate = 4.0");
    EXPECT_FAIL("mode = \"accuracy\", acc = 0.001, prec = 16");
    EXPECT_FAIL("mode = \"accuracy\", acc = 0.001, minbits = 0, maxbits = 64, maxprec = 32, minexp = -20");
    EXPECT_FAIL("mode = \"accuracy\", acc = 0.001, reversible = true");

    /* expert mode + alien keys */
    EXPECT_FAIL("mode = \"expert\", minbits = 0, maxbits = 64, maxprec = 32, minexp = -20, rate = 4.0");
    EXPECT_FAIL("mode = \"expert\", minbits = 0, maxbits = 64, maxprec = 32, minexp = -20, prec = 16");
    EXPECT_FAIL("mode = \"expert\", minbits = 0, maxbits = 64, maxprec = 32, minexp = -20, acc = 0.001");
    EXPECT_FAIL("mode = \"expert\", minbits = 0, maxbits = 64, maxprec = 32, minexp = -20, reversible = true");

    /* reversible mode + alien keys */
    EXPECT_FAIL("mode = \"reversible\", rate = 4.0");
    EXPECT_FAIL("mode = \"reversible\", prec = 16");
    EXPECT_FAIL("mode = \"reversible\", acc = 0.001");
    EXPECT_FAIL("mode = \"reversible\", minbits = 0, maxbits = 64, maxprec = 32, minexp = -20");

    /* mode value too long — must not truncate-match any known mode name */
    EXPECT_FAIL("mode = \"reversibleXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\"");

#undef EXPECT_FAIL

    return 0;
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    H5Z_zfp_initialize();

    if (test_set_config())           return 1;
    if (test_mode_inference())       return 1;
    if (test_full_roundtrip())       return 1;
#if ZFP_VERSION_NO >= 0x0550
    if (test_expert_introspection()) return 1;
#endif
    if (test_e2e())                  return 1;
    if (test_get_config_from_file()) return 1;
    if (test_errors())               return 1;
    if (test_locale_float())         return 1;
    if (test_conflict_matrix())      return 1;

    H5close();
    return 0;
}

#else /* H5Z_ZFP_USE_CLASS3 */

int main(void)
{
    /* This test requires H5ZFP_USE_CLASS3=ON */
    return 77; /* CTest SKIP exit code */
}

#endif /* H5Z_ZFP_USE_CLASS3 */
