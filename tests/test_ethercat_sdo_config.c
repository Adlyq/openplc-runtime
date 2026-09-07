/**
 * @file test_ethercat_sdo_config.c
 * @brief Unit tests for SDO value parsing in ecat_config_parse()
 *
 * Verifies that the parser correctly handles SDO values as both
 * JSON numbers and JSON strings (decimal, hex, float, negative).
 * Exercises the get_numeric_value() helper via ecat_config_parse().
 */

#include "ethercat_config.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

TEST_SOURCE_FILE("core/src/drivers/plugins/native/ethercat/cjson/cJSON.c")

void setUp(void) {}
void tearDown(void) {}

/**
 * Helper: write a minimal JSON config with a single slave that has one SDO
 * entry whose "value" field is set to the provided raw JSON token.
 * Returns the path to the temp file.
 */
static const char *TEMP_FILE = "test_sdo_config_tmp.json";

static int write_sdo_json(const char *value_token, const char *data_type)
{
    FILE *fp = fopen(TEMP_FILE, "w");
    if (!fp)
        return -1;

    fprintf(fp,
            "[{\n"
            "  \"name\": \"test\",\n"
            "  \"protocol\": \"ETHERCAT\",\n"
            "  \"config\": {\n"
            "    \"master\": { \"interface\": \"eth0\", \"cycle_time_us\": 1000, "
            "\"receive_timeout_us\": 2000 },\n"
            "    \"slaves\": [{\n"
            "      \"position\": 1,\n"
            "      \"name\": \"TestSlave\",\n"
            "      \"type\": \"coupler\",\n"
            "      \"vendor_id\": \"0x00000002\",\n"
            "      \"product_code\": \"0x00000001\",\n"
            "      \"revision\": \"0x00000001\",\n"
            "      \"channels\": [],\n"
            "      \"sdo_configurations\": [{\n"
            "        \"index\": \"0x8000\",\n"
            "        \"subindex\": 1,\n"
            "        \"value\": %s,\n"
            "        \"data_type\": \"%s\",\n"
            "        \"name\": \"TestSDO\"\n"
            "      }],\n"
            "      \"rx_pdos\": [],\n"
            "      \"tx_pdos\": []\n"
            "    }],\n"
            "    \"diagnostics\": {}\n"
            "  }\n"
            "}]\n",
            value_token, data_type);

    fclose(fp);
    return 0;
}

static void cleanup_temp(void)
{
    remove(TEMP_FILE);
}

/* ---- JSON number values ---- */

void test_sdo_parse_NumberValue_ShouldStoreCorrectly(void)
{
    write_sdo_json("100", "UINT16");

    /* ecat_config_t is too large for the stack; use static */
    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, config.slave_count);
    TEST_ASSERT_EQUAL_INT(1, config.slaves[0].sdo_count);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 100.0, config.slaves[0].sdo_configs[0].value);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_UINT16, config.slaves[0].sdo_configs[0].parsed_type);

    cleanup_temp();
}

void test_sdo_parse_NegativeNumber_ShouldStoreCorrectly(void)
{
    write_sdo_json("-50", "INT16");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -50.0, config.slaves[0].sdo_configs[0].value);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_INT16, config.slaves[0].sdo_configs[0].parsed_type);

    cleanup_temp();
}

void test_sdo_parse_FloatNumber_ShouldStoreCorrectly(void)
{
    write_sdo_json("3.14", "REAL");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14, config.slaves[0].sdo_configs[0].value);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_REAL32, config.slaves[0].sdo_configs[0].parsed_type);

    cleanup_temp();
}

/* ---- JSON string values (backward compatibility with old editor) ---- */

void test_sdo_parse_StringDecimal_ShouldParseAsNumber(void)
{
    write_sdo_json("\"100\"", "UINT16");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 100.0, config.slaves[0].sdo_configs[0].value);

    cleanup_temp();
}

void test_sdo_parse_StringHex_ShouldParseAsNumber(void)
{
    write_sdo_json("\"0xFF\"", "UINT8");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 255.0, config.slaves[0].sdo_configs[0].value);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_UINT8, config.slaves[0].sdo_configs[0].parsed_type);

    cleanup_temp();
}

void test_sdo_parse_StringFloat_ShouldParseAsNumber(void)
{
    write_sdo_json("\"3.14\"", "REAL32");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14, config.slaves[0].sdo_configs[0].value);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_REAL32, config.slaves[0].sdo_configs[0].parsed_type);

    cleanup_temp();
}

void test_sdo_parse_StringNegative_ShouldParseAsNumber(void)
{
    write_sdo_json("\"-50\"", "INT16");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, -50.0, config.slaves[0].sdo_configs[0].value);

    cleanup_temp();
}

void test_sdo_parse_StringEmpty_ShouldDefaultToZero(void)
{
    write_sdo_json("\"\"", "UINT16");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, config.slaves[0].sdo_configs[0].value);

    cleanup_temp();
}

/* ---- Missing value field ---- */

void test_sdo_parse_MissingValue_ShouldDefaultToZero(void)
{
    /* Write JSON without a "value" field */
    FILE *fp = fopen(TEMP_FILE, "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "[{\n"
                "  \"name\": \"test\",\n"
                "  \"protocol\": \"ETHERCAT\",\n"
                "  \"config\": {\n"
                "    \"master\": { \"interface\": \"eth0\", \"cycle_time_us\": 1000, "
                "\"receive_timeout_us\": 2000 },\n"
                "    \"slaves\": [{\n"
                "      \"position\": 1,\n"
                "      \"name\": \"TestSlave\",\n"
                "      \"type\": \"coupler\",\n"
                "      \"vendor_id\": \"0x00000002\",\n"
                "      \"product_code\": \"0x00000001\",\n"
                "      \"revision\": \"0x00000001\",\n"
                "      \"channels\": [],\n"
                "      \"sdo_configurations\": [{\n"
                "        \"index\": \"0x8000\",\n"
                "        \"subindex\": 1,\n"
                "        \"data_type\": \"UINT16\",\n"
                "        \"name\": \"TestSDO\"\n"
                "      }],\n"
                "      \"rx_pdos\": [],\n"
                "      \"tx_pdos\": []\n"
                "    }],\n"
                "    \"diagnostics\": {}\n"
                "  }\n"
                "}]\n");
    fclose(fp);

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, config.slaves[0].sdo_configs[0].value);

    cleanup_temp();
}

/* ---- Data type resolution in SDO ---- */

void test_sdo_parse_DataType_ShouldResolveParsedType(void)
{
    write_sdo_json("42", "DINT");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_INT32, config.slaves[0].sdo_configs[0].parsed_type);

    cleanup_temp();
}

void test_sdo_parse_LrealDataType_ShouldResolveReal64(void)
{
    write_sdo_json("1.5", "LREAL");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_REAL64, config.slaves[0].sdo_configs[0].parsed_type);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 1.5, config.slaves[0].sdo_configs[0].value);

    cleanup_temp();
}

/* ---- Large hex value ---- */

void test_sdo_parse_StringLargeHex_ShouldParseCorrectly(void)
{
    write_sdo_json("\"0x1A2B\"", "UINT16");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 6699.0, config.slaves[0].sdo_configs[0].value);

    cleanup_temp();
}

/* ---- Unrecognized data_type with a numeric bit_length ---- */
/* Editor exports of record objects (e.g. "DT2000EN8") carry no scalar type
 * name.  The parser maps them by bit_length and flags them best-effort so a
 * device rejection does not abort the bus. */

static int write_sdo_json_bit_length(const char *value_token, const char *data_type, int bit_length)
{
    FILE *fp = fopen(TEMP_FILE, "w");
    if (!fp)
        return -1;

    fprintf(fp,
            "[{\n"
            "  \"name\": \"test\",\n"
            "  \"protocol\": \"ETHERCAT\",\n"
            "  \"config\": {\n"
            "    \"master\": { \"interface\": \"eth0\", \"cycle_time_us\": 1000, "
            "\"receive_timeout_us\": 2000 },\n"
            "    \"slaves\": [{\n"
            "      \"position\": 1,\n"
            "      \"name\": \"TestSlave\",\n"
            "      \"type\": \"coupler\",\n"
            "      \"vendor_id\": \"0x00000002\",\n"
            "      \"product_code\": \"0x00000001\",\n"
            "      \"revision\": \"0x00000001\",\n"
            "      \"channels\": [],\n"
            "      \"sdo_configurations\": [{\n"
            "        \"index\": \"0x8000\",\n"
            "        \"subindex\": 1,\n"
            "        \"value\": %s,\n"
            "        \"data_type\": \"%s\",\n"
            "        \"bit_length\": %d,\n"
            "        \"name\": \"TestSDO\"\n"
            "      }],\n"
            "      \"rx_pdos\": [],\n"
            "      \"tx_pdos\": []\n"
            "    }],\n"
            "    \"diagnostics\": {}\n"
            "  }\n"
            "}]\n",
            value_token, data_type, bit_length);

    fclose(fp);
    return 0;
}

void test_sdo_parse_UnknownTypeWithBitLength_ShouldMapToUint8AndBestEffort(void)
{
    write_sdo_json_bit_length("0", "DT2000EN8", 8);

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, config.slaves[0].sdo_count);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_UINT8, config.slaves[0].sdo_configs[0].parsed_type);
    TEST_ASSERT_TRUE(config.slaves[0].sdo_configs[0].best_effort);

    cleanup_temp();
}

void test_sdo_parse_UnknownTypeWith16BitLength_ShouldMapToUint16(void)
{
    write_sdo_json_bit_length("0", "DT0800EN16", 16);

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_EQUAL_INT(ECAT_DTYPE_UINT16, config.slaves[0].sdo_configs[0].parsed_type);
    TEST_ASSERT_TRUE(config.slaves[0].sdo_configs[0].best_effort);

    cleanup_temp();
}

/* ---- Unrecognized structured data_type without usable width ---- */
/* Structured/array objects ("ARRAY [0..69] OF BYTE") cannot be encoded as a
 * scalar startup SDO write; the entry is skipped and parsing continues. */

void test_sdo_parse_StructuredArrayType_ShouldSkipEntryAndParse(void)
{
    FILE *fp = fopen(TEMP_FILE, "w");
    TEST_ASSERT_NOT_NULL(fp);

    fprintf(fp, "[{\n"
                "  \"name\": \"test\",\n"
                "  \"protocol\": \"ETHERCAT\",\n"
                "  \"config\": {\n"
                "    \"master\": { \"interface\": \"eth0\", \"cycle_time_us\": 1000, "
                "\"receive_timeout_us\": 2000 },\n"
                "    \"slaves\": [{\n"
                "      \"position\": 1,\n"
                "      \"name\": \"TestSlave\",\n"
                "      \"type\": \"coupler\",\n"
                "      \"vendor_id\": \"0x00000002\",\n"
                "      \"product_code\": \"0x00000001\",\n"
                "      \"revision\": \"0x00000001\",\n"
                "      \"channels\": [],\n"
                "      \"sdo_configurations\": [{\n"
                "        \"index\": \"0x8000\",\n"
                "        \"subindex\": 1,\n"
                "        \"data_type\": \"ARRAY [0..69] OF BYTE\",\n"
                "        \"name\": \"TestSDO\"\n"
                "      }],\n"
                "      \"rx_pdos\": [],\n"
                "      \"tx_pdos\": []\n"
                "    }],\n"
                "    \"diagnostics\": {}\n"
                "  }\n"
                "}]\n");
    fclose(fp);

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_EQUAL_INT(0, config.slaves[0].sdo_count);

    cleanup_temp();
}

/* ---- PAD stays invalid for an SDO ---- */

void test_sdo_parse_PadDataType_ShouldReject(void)
{
    write_sdo_json("0", "PAD");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_NOT_EQUAL_INT(ECAT_CONFIG_OK, rc);

    cleanup_temp();
}

/* ---- apply_after_operational flag ---- */

/**
 * Write a single-SDO JSON config with an explicit "apply_after_operational"
 * JSON value (true / false / omitted handled by a NULL token).
 */
static int write_sdo_json_flag(const char *flag_token)
{
    FILE *fp = fopen(TEMP_FILE, "w");
    if (!fp)
        return -1;

    fprintf(fp,
            "[{\n"
            "  \"name\": \"test\",\n"
            "  \"protocol\": \"ETHERCAT\",\n"
            "  \"config\": {\n"
            "    \"master\": { \"interface\": \"eth0\", \"cycle_time_us\": 1000, "
            "\"receive_timeout_us\": 2000 },\n"
            "    \"slaves\": [{\n"
            "      \"position\": 1,\n"
            "      \"name\": \"TestSlave\",\n"
            "      \"type\": \"coupler\",\n"
            "      \"vendor_id\": \"0x00000002\",\n"
            "      \"product_code\": \"0x00000001\",\n"
            "      \"revision\": \"0x00000001\",\n"
            "      \"channels\": [],\n"
            "      \"sdo_configurations\": [{\n"
            "        \"index\": \"0x8000\",\n"
            "        \"subindex\": 37,\n"
            "        \"value\": 3,\n"
            "        \"data_type\": \"UINT32\",\n"
            "        \"name\": \"MasterControl\",\n"
            "        %s\n"
            "      }],\n"
            "      \"rx_pdos\": [],\n"
            "      \"tx_pdos\": []\n"
            "    }],\n"
            "    \"diagnostics\": {}\n"
            "  }\n"
            "}]\n",
            flag_token ? flag_token : "\"apply_after_operational\": false");

    fclose(fp);
    return 0;
}

void test_sdo_parse_ApplyAfterOperationalTrue_ShouldStoreFlag(void)
{
    write_sdo_json_flag("\"apply_after_operational\": true");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_EQUAL_INT(1, config.slaves[0].sdo_count);
    TEST_ASSERT_TRUE(config.slaves[0].sdo_configs[0].apply_after_operational);

    cleanup_temp();
}

void test_sdo_parse_ApplyAfterOperationalFalse_ShouldStoreClearFlag(void)
{
    write_sdo_json_flag("\"apply_after_operational\": false");

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_FALSE(config.slaves[0].sdo_configs[0].apply_after_operational);

    cleanup_temp();
}

void test_sdo_parse_ApplyAfterOperationalMissing_ShouldDefaultToFalse(void)
{
    /* Same as write_sdo_json_flag(NULL): a legacy config without the key must
     * keep behaving as before (activation decided by the index fallback). */
    write_sdo_json_flag(NULL);

    static ecat_config_t config;
    int rc = ecat_config_parse(TEMP_FILE, &config);

    TEST_ASSERT_EQUAL_INT(ECAT_CONFIG_OK, rc);
    TEST_ASSERT_FALSE(config.slaves[0].sdo_configs[0].apply_after_operational);

    cleanup_temp();
}
