#pragma once

#include <IO/S3/PocoHTTPClient.h>
#include <IO/HTTPHeaderEntries.h>

#include <string>
#include <optional>

#include "config.h"

#if USE_AWS_S3

#include <base/types.h>
#include <Common/Exception.h>
#include <Common/Throttler_fwd.h>

#include <Poco/URI.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Errors.h>


namespace Aws::S3 { class S3Client; }

namespace DB
{

namespace ErrorCodes
{
    extern const int S3_ERROR;
}

class RemoteHostFilter;

class S3Exception : public Exception
{
public:

    // Format message with fmt::format, like the logging functions.
    template <typename... Args>
    S3Exception(Aws::S3::S3Errors code_, fmt::format_string<Args...> fmt, Args &&... args)
        : Exception(fmt::format(fmt, std::forward<Args>(args)...), ErrorCodes::S3_ERROR)
        , code(code_)
    {
    }

    S3Exception(const std::string & msg, Aws::S3::S3Errors code_)
        : Exception(msg, ErrorCodes::S3_ERROR)
        , code(code_)
    {}

    Aws::S3::S3Errors getS3ErrorCode() const
    {
        return code;
    }

    bool isRetryableError() const;

private:
    Aws::S3::S3Errors code;
};
}


namespace DB::S3
{

class ClientFactory
{
public:
    ~ClientFactory();

    static ClientFactory & instance();

    std::unique_ptr<Aws::S3::S3Client> create(
        const PocoHTTPClientConfiguration & cfg,
        bool is_virtual_hosted_style,
        const String & access_key_id,
        const String & secret_access_key,
        const String & server_side_encryption_customer_key_base64,
        HTTPHeaderEntries headers,
        bool use_environment_credentials,
        bool use_insecure_imds_request);

    PocoHTTPClientConfiguration createClientConfiguration(
        const String & force_region,
        const RemoteHostFilter & remote_host_filter,
        unsigned int s3_max_redirects,
        bool enable_s3_requests_logging,
        bool for_disk_s3,
        const ThrottlerPtr & get_request_throttler,
        const ThrottlerPtr & put_request_throttler);

private:
    ClientFactory();

    Aws::SDKOptions aws_options;
    std::atomic<bool> s3_requests_logging_enabled;
};

/**
 * Represents S3 URI.
 *
 * The following patterns are allowed:
 * s3://bucket/key
 * http(s)://endpoint/bucket/key
 */
struct URI
{
    Poco::URI uri;
    // Custom endpoint if URI scheme is not S3.
    String endpoint;
    String bucket;
    String key;
    String version_id;
    String storage_name;

    bool is_virtual_hosted_style;

    explicit URI(const std::string & uri_);

    static void validateBucket(const String & bucket, const Poco::URI & uri);
};

/// WARNING: Don't use `HeadObjectRequest`! Use the functions below instead.
/// Explanation: The `HeadObject` request never returns a response body (even if there is an error) however
/// if the request was sent without specifying a region in the endpoint (i.e. for example "https://test.s3.amazonaws.com/mydata.csv"
/// instead of "https://test.s3-us-west-2.amazonaws.com/mydata.csv") then that response body is one of the main ways
/// to determine the correct region and try to repeat the request again with the correct region.
/// For any other request type (`GetObject`, `ListObjects`, etc.) AWS SDK does that because they have response bodies,
/// but for `HeadObject` there is no response body so this way doesn't work.
/// That's why it's better to avoid using `HeadObject` requests at all.
/// See https://github.com/aws/aws-sdk-cpp/issues/1558 and also the function S3ErrorMarshaller::ExtractRegion() for more details.

struct ObjectInfo
{
    size_t size = 0;
    time_t last_modification_time = 0;
};

S3::ObjectInfo getObjectInfo(const Aws::S3::S3Client & client, const String & bucket, const String & key, const String & version_id = "", bool for_disk_s3 = false, bool throw_on_error = true);

size_t getObjectSize(const Aws::S3::S3Client & client, const String & bucket, const String & key, const String & version_id = "", bool for_disk_s3 = false, bool throw_on_error = true);

bool objectExists(const Aws::S3::S3Client & client, const String & bucket, const String & key, const String & version_id = "", bool for_disk_s3 = false, bool throw_on_error = true);

/// Checks if the object exists. If it doesn't exists the function returns an error without throwing any exception.
std::pair<bool /* exists */, Aws::S3::S3Error> checkObjectExists(const Aws::S3::S3Client & client, const String & bucket, const String & key, const String & version_id = "", bool for_disk_s3 = false);

bool isNotFoundError(Aws::S3::S3Errors error);

/// Returns the object's metadata.
std::map<String, String> getObjectMetadata(const Aws::S3::S3Client & client, const String & bucket, const String & key, const String & version_id = "", bool for_disk_s3 = false, bool throw_on_error = true);

}
#endif

namespace Poco::Util
{
    class AbstractConfiguration;
};

namespace DB::S3
{

struct AuthSettings
{
    static AuthSettings loadFromConfig(const std::string & config_elem, const Poco::Util::AbstractConfiguration & config);

    std::string access_key_id;
    std::string secret_access_key;
    std::string region;
    std::string server_side_encryption_customer_key_base64;

    HTTPHeaderEntries headers;

    std::optional<bool> use_environment_credentials;
    std::optional<bool> use_insecure_imds_request;

    bool operator==(const AuthSettings & other) const = default;

    void updateFrom(const AuthSettings & from);
};

}
