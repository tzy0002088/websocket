#ifndef __TLS_CERTIFICATE_H__
#define __TLS_CERTIFICATE_H__

#include <stddef.h>

/* 本地测试用自签名 CA(CN=127.0.0.1, 有效期 2026-10-07 ~ 2036-10-04),
 * 由 openssl 生成, 证书文件见 sample/hello-server/wss_cert.pem, 私钥见 wss_key.pem.
 * 仅用于本地 wss 测试, 请勿在生产环境使用.
 * 重新生成: openssl req -x509 -newkey rsa:2048 -keyout wss_key.pem -out wss_cert.pem \
 *   -days 3650 -nodes -subj "/CN=127.0.0.1" \
 *   -addext "basicConstraints=critical,CA:TRUE" \
 *   -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"
 */
#define MBEDTLS_CERTIFICATE    \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIDJTCCAg2gAwIBAgIUMrM7BX9lP0BjGRygKqgDKe4LYUwwDQYJKoZIhvcNAQEL\r\n" \
"BQAwFDESMBAGA1UEAwwJMTI3LjAuMC4xMB4XDTI2MTAwNzE1MDgyM1oXDTM2MTAw\r\n" \
"NDE1MDgyM1owFDESMBAGA1UEAwwJMTI3LjAuMC4xMIIBIjANBgkqhkiG9w0BAQEF\r\n" \
"AAOCAQ8AMIIBCgKCAQEAtpmFuE1jfUL6Z0D+JU5w95TDyWA+jSLsJnu696a8+E1F\r\n" \
"WS6/FMHKIK0MCrCNTkfoG4cxZvqWuRYCNaCCI8mTjD+jMhryDRgJpzu4oQTIleO/\r\n" \
"9ss6vOIBLzICsfToUZ1IXS/KwsahmmuXdfWrl+pGdq5K/hW/RurWhK8YGevQN2Dt\r\n" \
"GnCdpotYtlwz47tQ2Iehlym+qkCOdCixQF8KaXm7iy4PX5VnKreXE8XOim6Orm7j\r\n" \
"+0HAtDLechYMyFytCWnwOKdpKCJfNuAdNwS8iGj3HNz3ce3t3PtpcNtxr6BF61MR\r\n" \
"edy3XmqRGE4o1ma4ZkWEhmq5SliDe/KuFL6MVFKRzwIDAQABo28wbTAdBgNVHQ4E\r\n" \
"FgQUBIPVL+T4lm8Uska+Sq8iqeB6kY0wHwYDVR0jBBgwFoAUBIPVL+T4lm8Uska+\r\n" \
"Sq8iqeB6kY0wDwYDVR0TAQH/BAUwAwEB/zAaBgNVHREEEzARhwR/AAABgglsb2Nh\r\n" \
"bGhvc3QwDQYJKoZIhvcNAQELBQADggEBAA7TaKdmIVyMBrbVPlg1E5yS7D3QnOE6\r\n" \
"XLNgFq5QCUm0a6e8mJNJZ7iaMouKY5NdFAEc1vAfm70HqZD25a8s+wOKkzBMUr99\r\n" \
"acxOX2x81h48kAuCAv1SSZl8VYwt0Ttk7HxNVue6BZa3HDIjReMhY3J8TDDFJCkf\r\n" \
"HUd8Smb3+4fRBIttghKO/yfYujoMUw7qRMN+JTxJtAqu1GQG7YeCRV62esvaWdZX\r\n" \
"eRDfGbNkZu2JVXKXx7YP+POzh/ozePFEMroaKVZwewg56FHsG3HLt4F5tZxjO+Qd\r\n" \
"rzDGPAVP1zeSqL1ZIr6lWyoLAhJgVuCHT/fto/eanZivpycTjpk1SWQ=\r\n" \
"-----END CERTIFICATE-----\r\n"


extern const char mbedtls_certificate[];
extern const size_t mbedtls_certificate_len;

#endif //__TLS_CERTIFICATE_H__
