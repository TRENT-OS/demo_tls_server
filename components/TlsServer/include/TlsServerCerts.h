/*
 * Certificates and key for TLS server.
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#define TLS_SERVER_ROOT_CERT                                               \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIDrTCCApWgAwIBAgIUb63F+BTbaucqFFOSCu/HwcPalqMwDQYJKoZIhvcNAQEL\r\n" \
    "BQAwZjELMAkGA1UEBhMCREUxDzANBgNVBAgMBkJheWVybjESMBAGA1UEBwwJT3R0\r\n" \
    "b2JydW5uMRwwGgYDVQQKDBNIRU5TT0xEVCBDeWJlciBHbWJIMRQwEgYDVQQDDAtk\r\n" \
    "ZXYtaGMtcm9vdDAeFw0yMTA4MzAwNzQzMzhaFw0yMjA4MzAwNzQzMzhaMGYxCzAJ\r\n" \
    "BgNVBAYTAkRFMQ8wDQYDVQQIDAZCYXllcm4xEjAQBgNVBAcMCU90dG9icnVubjEc\r\n" \
    "MBoGA1UECgwTSEVOU09MRFQgQ3liZXIgR21iSDEUMBIGA1UEAwwLZGV2LWhjLXJv\r\n" \
    "b3QwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDCoJDXsZQL3KjkCyYd\r\n" \
    "kAIbaZwnx0t7JqM7c/HjrPiU2/i/elY7JmzikzUwLCpcG0wQT185TCTybOWh10es\r\n" \
    "bwdeyhwTtzAgFVzp8YYGyJ+UYpToa4cxb74la2NXV8g66qSC6ChwoOO4ILWMMHIS\r\n" \
    "2PfFZQvs9POS4PFy3x5kbsdzxtQ2eucsagAwQ8QlxNEBgFaqWLnAmSptdAo3ToTl\r\n" \
    "inZ6IcMKjX6W4LSvWYosR0ls8rzA1GzAIw/LK2XaEKGd0WtVVdnEAYgUJzbguC1U\r\n" \
    "pm3X15yncgrF3nRMvEi0YtApwp9NVDtxjabw5qEzy6bUZJn1U5EpX7twcJPRHl6Y\r\n" \
    "ByhTAgMBAAGjUzBRMB0GA1UdDgQWBBSsYHyPP6MLz/8gjK1in5QsD9yBZjAfBgNV\r\n" \
    "HSMEGDAWgBSsYHyPP6MLz/8gjK1in5QsD9yBZjAPBgNVHRMBAf8EBTADAQH/MA0G\r\n" \
    "CSqGSIb3DQEBCwUAA4IBAQAPejHIFCC896MacWmBql+lCrcOFAYCmDS92NCQDlbz\r\n" \
    "K0nHjGsI+q0UJmm8F7qfzReenmKl8l4o5i9FqHHDaXHJjO+0sXEz60ZkFy/SaXmz\r\n" \
    "czba3rJAPAQAc3KY3QZxobWYSWso1FX9NT00g4whfrdWCJjDC65rV+0zvl0CBCBU\r\n" \
    "Kt5JlmT1Ywqozg2U9DCa99azNAG5YBeAVxBh+FIESP7SqWE1+EfKr8aIgRNhOF4z\r\n" \
    "p3laRmoVmmA9SP/Z3AYWKuGpHFyakLqq7h9EJSPv6k/eGxm9inwhXOHKu+ZMznQS\r\n" \
    "NE0nuBs/3Ekl/IGHyZbAkcWCPw3wk3MPj6Y7j35sCWF2\r\n"                     \
    "-----END CERTIFICATE-----\r\n"


#define TLS_SERVER_CERT                                                    \
    "-----BEGIN CERTIFICATE-----\r\n"                                      \
    "MIIDqDCCApCgAwIBAgIUEXjrlz3HUNv+UQBLwy5jY4+eJs4wDQYJKoZIhvcNAQEL\r\n" \
    "BQAwZjELMAkGA1UEBhMCREUxDzANBgNVBAgMBkJheWVybjESMBAGA1UEBwwJT3R0\r\n" \
    "b2JydW5uMRwwGgYDVQQKDBNIRU5TT0xEVCBDeWJlciBHbWJIMRQwEgYDVQQDDAtk\r\n" \
    "ZXYtaGMtcm9vdDAeFw0yMTA4MzAwNzQzMzhaFw0yMjA4MzAwNzQzMzhaMGgxCzAJ\r\n" \
    "BgNVBAYTAkRFMQ8wDQYDVQQIDAZCYXllcm4xEjAQBgNVBAcMCU90dG9icnVubjEc\r\n" \
    "MBoGA1UECgwTSEVOU09MRFQgQ3liZXIgR21iSDEWMBQGA1UEAwwNZGV2LWhjLXNl\r\n" \
    "cnZlcjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMdFxzHni6NRK/kY\r\n" \
    "FumSieyfM34qS8Qql6URvVqd9KbwC/jarpJSUu2xWgPyEtMexBAdnojAf5lMKPDy\r\n" \
    "C1HmW4PuJDVonqa+pVQLmmlewRHp2abt83g2+AVColgDOQlZ6hhIDgLubWcgUybv\r\n" \
    "gCC2XrPkMLOUdu9NkukBz26r6xLzK2hGLbUDcnklU1PCHDnPFIjXdVwcMg3MCCck\r\n" \
    "8UNpbHRrS5jzANZuMP+a7W8qSBSN8DRp6mU0wJv4mvroMiqr56ReHKpO0Qhogaon\r\n" \
    "uSD5nB3e9oubyNdklDUTjaEOA0EsjNfLeemlYY3dBWILGKn5DwWA3b/MeNCyTAAf\r\n" \
    "VCbD7VcCAwEAAaNMMEowHwYDVR0jBBgwFoAUrGB8jz+jC8//IIytYp+ULA/cgWYw\r\n" \
    "CQYDVR0TBAIwADALBgNVHQ8EBAMCBPAwDwYDVR0RBAgwBocErBEAATANBgkqhkiG\r\n" \
    "9w0BAQsFAAOCAQEAfUZyFJz1bGzs49vhvSWB9gHH16MICx1w6gsxYMhsw6dHcsd4\r\n" \
    "Oznp88FmsvHSbRBCfgkWgdV6TLI3aYBmde2GqJq55ZY6WJtwqzyjso3KcJepOrqD\r\n" \
    "Op3bQzX/r9JmJObqlAsdREdRwa3KJCHioJKuSiw8Z31FzbprCVB1djo3p5IjbDTB\r\n" \
    "omJ2/20ICU83YIyRnEjlK+5aTGkkSOVerH+TLlPU2UWPD/q8579bz+KIKpCfg2mY\r\n" \
    "60Fuc83h2Ab8ZZu4x5QRuTHZcjC2DKsq8QTF4jzYUva94MDFz+gWVF2VbCZKkYyW\r\n" \
    "vISr+vhHJvkiSadwjtvehtcT1l1iplO0G26ktQ==\r\n"                         \
    "-----END CERTIFICATE-----\r\n"

#define TLS_SERVER_KEY                                                     \
    "-----BEGIN RSA PRIVATE KEY-----\r\n"                                  \
    "MIIEpQIBAAKCAQEAx0XHMeeLo1Er+RgW6ZKJ7J8zfipLxCqXpRG9Wp30pvAL+Nqu\r\n" \
    "klJS7bFaA/IS0x7EEB2eiMB/mUwo8PILUeZbg+4kNWiepr6lVAuaaV7BEenZpu3z\r\n" \
    "eDb4BUKiWAM5CVnqGEgOAu5tZyBTJu+AILZes+Qws5R2702S6QHPbqvrEvMraEYt\r\n" \
    "tQNyeSVTU8IcOc8UiNd1XBwyDcwIJyTxQ2lsdGtLmPMA1m4w/5rtbypIFI3wNGnq\r\n" \
    "ZTTAm/ia+ugyKqvnpF4cqk7RCGiBqie5IPmcHd72i5vI12SUNRONoQ4DQSyM18t5\r\n" \
    "6aVhjd0FYgsYqfkPBYDdv8x40LJMAB9UJsPtVwIDAQABAoIBAQDC7KvEUjXSpLU5\r\n" \
    "7WmEQzatgrFRCbihg/RgoPCzsm092vQrEmbPdL3wCpr93w6w+5hYF1EbfgmS/9/Q\r\n" \
    "iUOvcoE0lX9Pyy3d+AErLEp4JhsAFds1IfAWONb19k9tfoGNdym5ZMpn7aiQxxrv\r\n" \
    "rDmORjZvC5jkIScSQLSjPoUVQhApsH/VjX1lhftnnV2noltazlCXB3JV2uh2IZuQ\r\n" \
    "K2dza4E6LFtPKiiDqxLZN0I/IHk8OykHSHYPnAACGR7fHmT9nYYWugyZfcaws4fn\r\n" \
    "iK9lActVNG3qaf2zvV309iY3wbaFXChuZJhylJs3a4pNlyuskCjh9CE0GJoWWbO8\r\n" \
    "joWRIhKJAoGBAO48sQokprztPEXiyldIfqm5YXNS91LIOtLSuHGqUQu68esHub9x\r\n" \
    "dQ+rhVty/ZwCkl6yOGwzs2J+T01n1U8aAskNuvikEfQpSEQ0mB355vy0kG9bZLSm\r\n" \
    "pPIBD37CTgenO/M1cXaQ9hzg4+6wxZdrva2jXhbySdK1i/AhkPqqB7jtAoGBANYh\r\n" \
    "W746siN9Ax6uMosH4r41p2p3yTxwmFJXmDsX4B0By31w4Di5wc8BQyXwrchodeUU\r\n" \
    "PkIbBc7Ieams8gmWsMOOPR856fHWr+nXvxdJTHwO93ww/Hn0D88MAZ4RLIUu2ieM\r\n" \
    "+7UWq7kWxOOW4bj9rz2ssfThfmQoEKmsm0V+ekrTAoGBAMHyRMqWJeu+UoguZj2C\r\n" \
    "diNkGNKS42fQPBsvkxpt5kbfbVzCUFRrYDpej1VwmsgsS86t1kM4H7x6ScMhWcVo\r\n" \
    "zoWxGNqcb0VPalakXoZg0Mw/jyooxCZRWAzwEhZGxtFyMtr/UhyNTN8bslO6M1Hf\r\n" \
    "U26Nhea2XqUcSQ03tlhqnZjNAoGAc2Uqwctz8LgQFFqgFli7kvHrNO803YN4MvfN\r\n" \
    "rBrjxf4PoZxQ2YERtvLhMvMPVC8nSbqtCobxjExxdEUlcpFo1Ro0Sj21m4Ss7II+\r\n" \
    "EtiHhVuzd5QWm8oxMs0vmfV7XpKvMh1CEIcVJ/vjQxsurbjY1Y3ZoTRcHrGQuT+x\r\n" \
    "tbPBR80CgYEAr5L2mJs2DfhNARImkm2th4bgJTY4Zgdb3msKUuKMHJSm7uKtuy0X\r\n" \
    "0vkhoaOelma5uDtzYv3lw9bBAnfYfZxjmftGM3olcM81PYHvpt8q8oOX0VYH4aVq\r\n" \
    "RrGvs99MyMJcqhY91HPCTeeIZdqpoScYSYe5gx//DoCbnbt/8aZe/4M=\r\n"         \
    "-----END RSA PRIVATE KEY-----\r\n"
