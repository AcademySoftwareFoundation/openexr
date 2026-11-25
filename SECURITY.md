<!-- SPDX-License-Identifier: BSD-3-Clause -->
<!-- Copyright (c) Contributors to the OpenEXR Project -->

# Security Policy

## Reporting a Vulnerability

If you think you've found a potential vulnerability in OpenEXR, please
report it by filing a GitHub [security
advisory](https://github.com/AcademySoftwareFoundation/openexr/security/advisories/new). Alternatively,
email security@openexr.com and provide your contact info for further
private/secure discussion.  If your email does not receive a prompt
acknowledgement, your address may be blocked.

Our policy is to acknowledge the receipt of vulnerability reports
within 48 hours. Our policy is to address critical security vulnerabilities
rapidly and post patches within 14 days if possible.

## CVE Assignment

Our criteria for requesting a CVE are:

- Memory safety violations (out-of-bounds write, use-after-free, heap
  corruption) reachable on mainstream 64-bit architectures through the
  public library API, confirmed in an unsanitized build on a mainstream
  platform, and affecting library code so that every downstream consumer
  inherits the risk.

We do not request a CVE for:

- 32-bit-only bugs — bugs on 32-bit systems are real and will be fixed,
  but since the vast majority of downstream packages (Linux distros,
  conda-forge, vcpkg) build for 64-bit exclusively, a CVE creates
  unnecessary work for maintainers who are not actually affected.

- Developer-tool-only crashes (exrmetrics, exrcheck) — the library
  itself and standard utility programs are safe; only users who run
  one of the developer tools on untrusted input are exposed.

Flaws whose root cause lies in a bundled dependency (such as OpenJPH)
are fixed upstream; the upstream project owns the CVE when one is
warranted. This applies even when a flaw is detected in dependency code
vendored into OpenEXR's `external/` source directory. OpenEXR addresses
such flaws by updating the dependency and noting the upstream advisory
in the release notes. We request a CVE for OpenEXR itself only when the
flaw is in our code and untrusted input can reach it through normal use
of the library.

## Known Vulnerabilities

| CVE | Affected Versions | Patched Versions |
| --- | ----------------- | ---------------- |
| [CVE-2026-68514](https://www.cve.org/CVERecord?id=CVE-2026-68514) | 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.3.13, 3.4.14 |
| [CVE-2026-68513](https://www.cve.org/CVERecord?id=CVE-2026-68513) | 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.3.13, 3.4.14 |
| [CVE-2026-62986](https://www.cve.org/CVERecord?id=CVE-2026-62986) | 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.3.13, 3.4.14 |
| [CVE-2026-61703](https://www.cve.org/CVERecord?id=CVE-2026-61703) | 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.3.13, 3.4.14 |
| [CVE-2026-61555](https://www.cve.org/CVERecord?id=CVE-2026-61555) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59985](https://www.cve.org/CVERecord?id=CVE-2026-59985) | 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59984](https://www.cve.org/CVERecord?id=CVE-2026-59984) | 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59983](https://www.cve.org/CVERecord?id=CVE-2026-59983) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59982](https://www.cve.org/CVERecord?id=CVE-2026-59982) | 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59981](https://www.cve.org/CVERecord?id=CVE-2026-59981) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59189](https://www.cve.org/CVERecord?id=CVE-2026-59189) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59187](https://www.cve.org/CVERecord?id=CVE-2026-59187) | 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.3.13, 3.4.14 |
| [CVE-2026-59186](https://www.cve.org/CVERecord?id=CVE-2026-59186) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59184](https://www.cve.org/CVERecord?id=CVE-2026-59184) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-59183](https://www.cve.org/CVERecord?id=CVE-2026-59183) | 3.1.x, 3.2.0–3.2.10, 3.3.0–3.3.12, 3.4.0–3.4.13 | 3.2.11, 3.3.13, 3.4.14 |
| [CVE-2026-55373](https://www.cve.org/CVERecord?id=CVE-2026-55373) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.9, 3.3.0–3.3.11, 3.4.0–3.4.12 | 3.2.10, 3.3.12, 3.4.13 |
| [CVE-2026-55371](https://www.cve.org/CVERecord?id=CVE-2026-55371) | 3.4.0–3.4.12 | 3.4.13 |
| [CVE-2026-55059](https://www.cve.org/CVERecord?id=CVE-2026-55059) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.9, 3.3.0–3.3.11, 3.4.0–3.4.12 | 3.2.10, 3.3.12, 3.4.13 |
| [CVE-2026-54920](https://www.cve.org/CVERecord?id=CVE-2026-54920) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.9, 3.3.0–3.3.11, 3.4.0–3.4.12 | 3.2.10, 3.3.12, 3.4.13 |
| [CVE-2026-53532](https://www.cve.org/CVERecord?id=CVE-2026-53532) | 3.4.0–3.4.12 | 3.4.13 |
| [CVE-2026-45696](https://www.cve.org/CVERecord?id=CVE-2026-45696) | 3.4.0–3.4.11 | 3.4.12 |
| [CVE-2026-44663](https://www.cve.org/CVERecord?id=CVE-2026-44663) | 3.4.0–3.4.11 | 3.4.12 |
| [CVE-2026-42217](https://www.cve.org/CVERecord?id=CVE-2026-42217) | 3.2.0–3.2.8, 3.3.0–3.3.10, 3.4.0–3.4.10 | 3.2.9, 3.3.11, 3.4.11 |
| [CVE-2026-42216](https://www.cve.org/CVERecord?id=CVE-2026-42216) | 3.2.0–3.2.8, 3.3.0–3.3.10, 3.4.0–3.4.10 | 3.2.9, 3.3.11, 3.4.11 |
| [CVE-2026-41142](https://www.cve.org/CVERecord?id=CVE-2026-41142) | 3.2.0–3.2.8, 3.3.0–3.3.10, 3.4.0–3.4.10 | 3.2.9, 3.3.11, 3.4.11 |
| [CVE-2026-40250](https://www.cve.org/CVERecord?id=CVE-2026-40250) | 3.2.0–3.2.7, 3.3.0–3.3.9, 3.4.0–3.4.9 | 3.2.8, 3.3.10, 3.4.10 |
| [CVE-2026-40244](https://www.cve.org/CVERecord?id=CVE-2026-40244) | 3.2.0–3.2.7, 3.3.0–3.3.9, 3.4.0–3.4.9 | 3.2.8, 3.3.10, 3.4.10 |
| [CVE-2026-39886](https://www.cve.org/CVERecord?id=CVE-2026-39886) | 3.4.0–3.4.9 | 3.4.10 |
| [CVE-2026-34589](https://www.cve.org/CVERecord?id=CVE-2026-34589) | 3.2.0–3.2.6, 3.3.0–3.3.8, 3.4.0–3.4.8 | 3.2.7, 3.3.9, 3.4.9 |
| [CVE-2026-34588](https://www.cve.org/CVERecord?id=CVE-2026-34588) | 3.1.x, 3.2.0–3.2.6, 3.3.0–3.3.8, 3.4.0–3.4.8 | 3.2.7, 3.3.9, 3.4.9 |
| [CVE-2026-34545](https://www.cve.org/CVERecord?id=CVE-2026-34545) | 3.4.0–3.4.6 | 3.4.7 |
| [CVE-2026-34544](https://www.cve.org/CVERecord?id=CVE-2026-34544) | 3.2.0–3.2.6, 3.3.0–3.3.8, 3.4.0–3.4.7 | 3.2.7, 3.3.9, 3.4.8 |
| [CVE-2026-34543](https://www.cve.org/CVERecord?id=CVE-2026-34543) | 3.2.0–3.2.6, 3.3.0–3.3.8, 3.4.0–3.4.7 | 3.2.7, 3.3.9, 3.4.8 |
| [CVE-2026-34380](https://www.cve.org/CVERecord?id=CVE-2026-34380) | 3.2.0–3.2.6, 3.3.0–3.3.8, 3.4.0–3.4.8 | 3.2.7, 3.3.9, 3.4.9 |
| [CVE-2026-34379](https://www.cve.org/CVERecord?id=CVE-2026-34379) | 3.2.0–3.2.6, 3.3.0–3.3.8, 3.4.0–3.4.8 | 3.2.7, 3.3.9, 3.4.9 |
| [CVE-2026-34378](https://www.cve.org/CVERecord?id=CVE-2026-34378) | 3.4.0–3.4.8 | 3.4.9 |
| [CVE-2026-27622](https://www.cve.org/CVERecord?id=CVE-2026-27622) | 2.x, 3.0.x, 3.1.x, 3.2.0–3.2.5, 3.3.0–3.3.7, 3.4.0–3.4.5 | 3.2.6, 3.3.8, 3.4.6 |
| [CVE-2026-26981](https://www.cve.org/CVERecord?id=CVE-2026-26981) | 3.3.0–3.3.6, 3.4.0–3.4.4 | 3.3.7, 3.4.5 |
| [CVE-2025-64183](https://www.cve.org/CVERecord?id=CVE-2025-64183) | 3.2.0–3.2.4, 3.3.0–3.3.5, 3.4.0–3.4.2 | 3.2.5, 3.3.6, 3.4.3 |
| [CVE-2025-64182](https://www.cve.org/CVERecord?id=CVE-2025-64182) | 3.2.0–3.2.4, 3.3.0–3.3.5, 3.4.0–3.4.2 | 3.2.5, 3.3.6, 3.4.3 |
| [CVE-2025-64181](https://www.cve.org/CVERecord?id=CVE-2025-64181) | 3.3.0–3.3.5, 3.4.0–3.4.2 | 3.3.6, 3.4.3 |
| [CVE-2025-48074](https://www.cve.org/CVERecord?id=CVE-2025-48074) | 3.3.2 | 3.3.3 |
| [CVE-2025-48073](https://www.cve.org/CVERecord?id=CVE-2025-48073) | 3.3.2 | 3.3.3 |
| [CVE-2025-48072](https://www.cve.org/CVERecord?id=CVE-2025-48072) | 3.3.2 | 3.3.3 |
| [CVE-2025-48071](https://www.cve.org/CVERecord?id=CVE-2025-48071) | 3.3.0–3.3.2 | 3.3.3 |
| [CVE-2025-12840](https://www.cve.org/CVERecord?id=CVE-2025-12840) | 3.3.0–3.3.5, 3.4.0–3.4.2 | 3.3.6, 3.4.3 |
| [CVE-2025-12839](https://www.cve.org/CVERecord?id=CVE-2025-12839) | 3.3.0–3.3.5, 3.4.0–3.4.2 | 3.3.6, 3.4.3 |
| [CVE-2025-12495](https://www.cve.org/CVERecord?id=CVE-2025-12495) | 3.3.0–3.3.5, 3.4.0–3.4.2 | 3.3.6, 3.4.3 |
| [CVE-2023-5841](https://nvd.nist.gov/vuln/detail/CVE-2023-5841) | 3.1.0–3.1.11, 3.2.0–3.2.1 | 3.2.2 |
| [CVE-2021-45942](https://nvd.nist.gov/vuln/detail/CVE-2021-45942) | 3.1.0–3.1.3 | 3.1.4 |
| [CVE-2021-20296](https://nvd.nist.gov/vuln/detail/CVE-2021-20296) | 2.4.0–2.4.2, 2.5.0–2.5.3 | 2.4.3, 2.5.4 |
| [CVE-2021-3479](https://nvd.nist.gov/vuln/detail/CVE-2021-3479) | 2.4.0–2.4.2, 2.5.0–2.5.3 | 2.4.3, 2.5.4 |
| [CVE-2021-3478](https://nvd.nist.gov/vuln/detail/CVE-2021-3478) | 2.4.0–2.4.2, 2.5.0–2.5.3 | 2.4.3, 2.5.4 |
| [CVE-2021-3477](https://nvd.nist.gov/vuln/detail/CVE-2021-3477) | 2.4.0–2.4.2, 2.5.0–2.5.3 | 2.4.3, 2.5.4 |
| [CVE-2021-3476](https://nvd.nist.gov/vuln/detail/CVE-2021-3476) | 2.4.0–2.4.2, 2.5.0–2.5.3 | 2.4.3, 2.5.4 |
| [CVE-2021-3475](https://nvd.nist.gov/vuln/detail/CVE-2021-3475) | 2.4.0–2.4.2, 2.5.0–2.5.3 | 2.4.3, 2.5.4 |
| [CVE-2021-3474](https://nvd.nist.gov/vuln/detail/CVE-2021-3474) | 2.4.0–2.4.2, 2.5.0–2.5.3 | 2.4.3, 2.5.4 |
| [CVE-2020-16589](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-16589) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0 | 2.4.0 |
| [CVE-2020-16588](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-16588) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0 | 2.4.0 |
| [CVE-2020-16587](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-16587) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0 | 2.4.0 |
| [CVE-2020-15306](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-15306) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0–2.4.1, 2.5.0–2.5.1 | 2.5.2 |
| [CVE-2020-15305](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-15305) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0–2.4.1, 2.5.0–2.5.1 | 2.5.2 |
| [CVE-2020-15304](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-15304) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0–2.4.1, 2.5.0–2.5.1 | 2.5.2 |
| [CVE-2020-11765](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11765) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2020-11764](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11764) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2020-11763](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11763) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2020-11762](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11762) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2020-11761](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11761) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2020-11760](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11760) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2020-11759](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11759) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2020-11758](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2020-11758) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0, 2.4.0 | 2.4.1 |
| [CVE-2018-18444](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2018-18444) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0 | 2.4.0 |
| [CVE-2018-18443](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2018-18443) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1, 2.3.0 | 2.4.0 |
| [CVE-2017-9116](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-9116) | 2.0.0–2.0.1, 2.1.0, 2.2.0 | 2.2.1 |
| [CVE-2017-9115](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-9115) | 2.0.0–2.0.1, 2.1.0, 2.2.0 | 2.2.1 |
| [CVE-2017-9114](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-9114) | 2.0.0–2.0.1, 2.1.0, 2.2.0 | 2.2.1 |
| [CVE-2017-9113](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-9113) | 2.0.0–2.0.1, 2.1.0, 2.2.0 | 2.2.1 |
| [CVE-2017-9112](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-9112) | 2.0.0–2.0.1, 2.1.0, 2.2.0 | 2.2.1 |
| [CVE-2017-9111](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-9111) | 2.0.0–2.0.1, 2.1.0, 2.2.0 | 2.2.1 |
| [CVE-2017-9110](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-9110) | 2.0.0–2.0.1, 2.1.0, 2.2.0 | 2.2.1 |
| [CVE-2017-12596](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2017-12596) | 2.0.0–2.0.1, 2.1.0, 2.2.0–2.2.1 | 2.3.0 |
| [CVE-2009-1722](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2009-1722) | 1.0.x, 1.1.x, 1.2.x, 1.3.x, 1.4.x, 1.5.x, 1.6.x | 1.7.0 |
| [CVE-2009-1721](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2009-1721) | 1.0.x, 1.1.x, 1.2.x, 1.3.x, 1.4.x, 1.5.x, 1.6.x | 1.7.0 |
| [CVE-2009-1720](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2009-1720) | 1.0.x, 1.1.x, 1.2.x, 1.3.x, 1.4.x, 1.5.x, 1.6.x | 1.7.0 |

See the [release notes](CHANGES.md) for more information.

## Supported Versions

This gives guidance about which releases/branches are supported with bug fixes and patches to
security vulnerabilities.

| Version / branch  | Supported                                            |
|-------| ---------------------------------------------------- |
| main  | :white_check_mark: :construction: All fixes immediately, although this branch is under development with potential unstable ABI/API
| 3.5.x | :white_check_mark: All fixes that can be backported without breaking ABI compatibility
| 3.4.x | :white_check_mark: All security-related patches that can be backported without breaking ABI compatibility
| 3.3.x | :white_check_mark: All security-related patches that can be backported without breaking ABI compatibility
| 3.2.x | :warning: Patch releases considered in response to specific requests
| 3.1.x | :warning: Patch releases considered in response to specific requests
| 3.0.x | :warning: Patch releases considered in response to specific requests
| 2.5.x | :warning: Patch releases considered in response to specific requests
| 2.4.x | :warning: Patch releases considered in response to specific requests
| 2.3.x | :x: No longer receiving patches of any kind
| 2.2.x | :x: No longer receiving patches of any kind
| 2.1.x | :x: No longer receiving patches of any kind
| 2.0.x | :x: No longer receiving patches of any kind
| 1.x   | :x: No longer receiving patches of any kind

## Signed Releases

Releases artifacts are signed via
[sigstore](https://www.sigstore.dev). See
[release-sign.yml](.github/workflows/release-sign.yml) for details.

To verify a downloaded release at a given tag:

    % pip install sigstore
    % sigstore verify github --cert-identity https://github.com/AcademySoftwareFoundation/openexr/.github/workflows/release-sign.yml@refs/tags/<tag> openexr-<tag>.tar.gz

## Security Expectations

### Software Features

- The OpenEXR project implements the EXR image file format, used
  throughout the motion picture industry and beyond, on Linux, macOS,
  and Windows.

- The project consists of a software run-time library, implemented in
  C/C++ and built via cmake, that reads and writes image data
  files. The project also distributes python wrappings for the C/C++
  I/O API.

- The library reads and writes binary image data and text-based
  metadata, treated as blind data, none of which is executable code.

- Other than the website and online technical documentation, the
  project implements no web/online services or network communication
  protocols.  The library never requests any security or
  authentication credentials or login information from users.

  The website implements no interactive features and requires no login
  credentials.

- The library reads and writes only to file paths specifically requested
  via the C/C++ API. The runtime library uses no system configuration
  files or sidecar data files. Access to data files uses only standard
  file I/O system calls.

- The library compresses/decompresses data via standard compression
  algorithms but uses no cryptographic or confidentiality protocols.

### Software Dependencies

OpenEXR depends on
[Imath](https://github.com/AcademySoftwareFoundation/Imath), a library
of basic math operations also maintained and distributed by the
OpenEXR project. Imath follows the same security conventions
documented here for OpenEXR itself. The core Imath library has no
external dependencies. The Imath python bindings depend on python and
boost.

OpenEXR has three external codec dependencies:
[libdeflate](https://github.com/ebiggers/libdeflate), which implements
standard deflate/zlib/gzip compression and decompression;
[OpenJPH](https://github.com/aous72/OpenJPH), which implements HTJ2K
compression; and [zstd](https://github.com/facebook/zstd), which
implements Zstandard compression.

The project uses
[Snyk](https://github.com/AcademySoftwareFoundation/openexr/blob/main/.github/workflows/snyk-scan-cron.yml)
to scan for dependency vulnerability.

### Potential Vulnerabilities

Potential entry points are images being loaded using the
library. Malformed images could cause issues such as heap buffer
overflows, out-of-memory faults, or segmentation faults that could be
exploitable as denial-of-service attacks.

### Image Size Limits and Out-of-Memory Failures

The OpenEXR file format places no fixed limit on image size, except
that image width and height are represented by signed 32-bit integers
and therefore technically limited to a maximum of 2,147,483,647.

Memory allocation failures caused by large image dimensions declared
in file headers are not considered security vulnerabilities when the
allocation size is proportional to the declared image dimensions. EXR
files can legitimately describe very large images, and the memory
required to decode them is inherently proportional to their pixel
count. Exhausting available memory on a given machine is a system
resource constraint, not a library defect — the same file that
triggers an out-of-memory error on one machine may load successfully
on another with more memory.

The OpenEXR library provides 
`Imf::Header::setMaxImageSize(int maxWidth,int maxHeight)` and
`Imf::Header::setMaxTileSize(int maxWidth,int maxHeight)` (and
`exr_set_default_maximum_image_size()` and
`exr_set_default_maximum_tile_size()` in OpenEXRCore) to allow
applications to reject files with dimensions exceeding a configurable
limit before any large allocation occurs. Applications processing
untrusted EXR files should set these limits to values appropriate for
their deployment environment.

### Secure Design Principles

The project applies secure design principles where they are applicable
to a file-format I/O library:

- **Economy of mechanism / minimized attack surface.** The library
  implements no network protocols, services, authentication, plugins,
  or configuration files, and treats all file contents as image pixel
  data and image metadata, never as executable code. There are no
  intentional mechanisms for OpenEXR to interpret attributes or image
  data as instructions to be executed as code. It depends on Imath for
  basic math constructs and has three external dependencies that
  implement compression codecs: libdeflate, OpenJPH, and zstd. It
  performs file access only for the paths the application requests.

- **Least privilege.** The library requires no privileges beyond
  reading and writing the files an application asks it to open. It
  never changes permissions, spawns processes, or reads credentials,
  and neither it nor its tools are installed setuid; none of the
  OpenEXR tool programs are installed to execute with special
  privileges.

- **Complete mediation.** All file access goes through a single I/O
  layer in OpenEXRCore (the context's read/write callbacks, which can
  also be replaced to mediate access to non-file sources). Every size,
  count, offset, and dimension read from a file is validated against
  the file size, the declared part, and format limits before it is used
  to read, allocate, or index. The C++ API is built on this same layer.

- **Fail-safe behavior.** Validation failures are reported through
  error codes or exceptions and the operation fails rather than
  continuing with partially valid data. Applications reading untrusted
  files should enable strict header validation
  (`EXR_CONTEXT_FLAG_STRICT_HEADER`) and set image and tile size
  limits as described under "Image Size Limits and Out-of-Memory
  Failures" above.

- **Open design.** The EXR file format and the implementation are
  public and open source, so security does not depend on secrecy of
  the design.  Security may be audited directly based on the design
  documentation and source code.

- **Defense in depth.** Robustness does not rely on any single
  control: input validation, continuous fuzzing (OSS-Fuzz), static
  analysis (CodeQL), memory-error testing (Valgrind), OpenSSF
  Scorecard monitoring, code review, and signed releases are used
  together, as described below.

- **Psychological acceptability.** Security-relevant behavior (strict
  mode, size limits) is exposed through the same ordinary API that
  applications already use to open files, with documented defaults,
  rather than through separate mechanisms.

The principle of separation of privilege does not apply, since the
library has no authentication, authorization, or multi-user mechanisms
of its own; those are left to the operating system and the embedding
application.

Where principles conflict, the project favors compatibility with the
existing file format over stricter defaults, which is why image and
tile size limits are opt-in by the application.

### Hardening

#### Testing

The OpenEXR project implements a comprehensive suite of validation
tests, including fuzz testing to harden against malicious input
data. Note that fuzz testing hardens only against *small* input data
files and is not a comprehensive test against all potential input.

Note that the
[exrcheck](https://github.com/AcademySoftwareFoundation/openexr/tree/main/src/bin/exrcheck)
utility is intended to be used by testers to demonstrate a particular
proof-of-concept input file exposes a vulnerability, and it is very
helpful to let us know if a vulnerability can be reproduced using that
tool.

The project also uses the [OSS
Fuzz](https://bugs.chromium.org/p/oss-fuzz) service for continuous
fuzz testing.

#### CI and Release Infrastructure

The project hardens its build and release infrastructure against
tampering:

- Every third-party GitHub Action is pinned to a full commit SHA, not a
  mutable tag, and Dependabot proposes updates daily so that each
  change is reviewed before it is adopted.
- Every workflow declares its `GITHUB_TOKEN` permissions explicitly,
  read-only by default. Write access is granted only to the specific
  jobs that need it, such as signing and uploading a release.
- Secrets are used only by the few workflows that require them, such as
  code-analysis and release notifications. No workflow uses
  `pull_request_target`, so code from a pull request does not run with
  access to repository secrets.
- Release tarballs are signed with Sigstore by a workflow, so their
  origin is verifiable (see "Signed Releases").
- The [OpenSSF Scorecard](https://securityscorecards.dev) workflow
  audits the repository's security practices weekly, and CodeQL
  analyzes the code on a schedule.
- Workflow files are changed through the same review as source code,
  by at least one maintainer.

#### Development Cycle and Distribution

OpenEXR is downloadable and buildable by C/C++ source via GitHub. Only
members of the project's Technical Steering Committee, all veteran
software engineers at major motion picture studios or vendors, have
write permissions on the source code repository. All critical software
changes are reviewed and approved by at least one maintainer before
they are merged.

The library is distributed in binary form via many common package
managers across all platforms.

## Assurance Case

This section argues that OpenEXR meets its security requirements. It
builds on "Security Expectations" and "Secure Design Principles" above.

### Security requirements

When reading a file, the library must:

1. never read or write memory outside the buffers it owns, however
   malformed the input;
2. never execute data from a file;
3. fail with an error or exception, not crash or hang, on any invalid
   input, and not fall back to guessing;
4. never access anything beyond the file paths the application
   requests.

The released artifacts must come from the project's reviewed source and
be verifiable.

### Threat model

**Assets.** The integrity and availability of the application that
embeds the library, and the data it processes.

**Attackers considered.**

- Anyone able to supply an EXR file to an application using OpenEXR as
  an input image format. The attacker controls every byte of the file
  and needs no credentials. The library has no authentication,
  network, or multi-user features, so there is no other attack
  channel.
- A malicious actor targeting the project's infrastructure, rather than
  an application that uses OpenEXR: someone who tries to subvert the
  repository by submitting malicious code or dependency changes, or to
  compromise the CI/CD pipeline, build environment, or release process
  in order to tamper with the source or the released artifacts. Such an
  attack could reach every user of a release, regardless of how
  carefully an application handles untrusted files.

**Threats considered.**

- Memory-safety violations (out-of-bounds read or write, use after
  free, integer overflow leading to an undersized allocation) caused
  by crafted sizes, counts, offsets, or compressed data.
- Denial of service: crashes, infinite loops, or memory or CPU
  exhaustion from a small crafted file.
- Compromised or tampered release artifacts, or a vulnerable
  dependency.
- Compromise of the CI/CD pipeline or release process, such as a
  malicious or hijacked GitHub Action, leaked credentials, or an
  over-privileged workflow token, used to inject code into a release
  (see "CI and Release Infrastructure").

**Out of scope.**

- Allocation failures proportional to the image dimensions declared in
  a valid file (see "Image Size Limits and Out-of-Memory
  Failures"). Because pixel data is stored compressed, an image with a
  large width and height can be represented by a file that is small on
  disk.
- The security of the embedding application and operating system.
- Confidentiality, since the library implements no cryptography.

### Trust boundaries

1. **File contents (untrusted) to library.** Every byte of an input
   file is untrusted. This is the only boundary that carries attacker
   data. It is mediated by one I/O layer in OpenEXRCore (see "Complete
   mediation" above); the C++ API, the Python bindings, and the tools
   all sit on top of that layer.
2. **Application to library (trusted).** The application is trusted to
   choose which files to open, set image and tile size limits, and
   enable strict header validation. Applications reading untrusted
   files are expected to do so.
3. **Library to dependencies.** Imath (maintained by this project) and
   the three codec libraries, libdeflate, OpenJPH, and zstd, are the
   only dependencies. The codecs receive compressed data from the file
   and are treated as part of the input path; the library checks the
   sizes they produce.
4. **Source to release.** Only members of the Technical Steering
   Committee may write to the repository, changes are reviewed by a
   maintainer before they are merged, and release artifacts are signed
   (see "Signed Releases").

### Applying the principles

The argument that secure design principles are applied is the "Secure
Design Principles" section above: minimized attack surface, least
privilege, complete mediation, fail-safe defaults, open design,
defense in depth, and psychological acceptability. The documented
trade-off is that image and tile size limits are opt-in by the
application, to preserve compatibility with existing files. Applications
that handle untrusted files should set them and enable strict header
validation.

### Common implementation weaknesses

Identified weaknesses for a binary-format parser written in C and C++
are listed below, with mitigations:

| Weakness | Countermeasure |
|---|---|
| Out-of-bounds read or write (CWE-125, CWE-787) | Offsets, sizes, and counts read from the file are validated against the file size and the part before use; tested by fuzzing and AddressSanitizer. |
| Integer overflow leading to undersized buffers (CWE-190, CWE-680) | Size computations use 64-bit arithmetic and overflow-checked helpers (e.g. `internal_util.h`). |
| NULL dereference, use after free, double free (CWE-476, CWE-416, CWE-415) | Allocation failures are checked and reported as errors; Valgrind and AddressSanitizer runs in CI. |
| Uncontrolled resource consumption (CWE-400, CWE-770) | Configurable maximum image and tile size; fuzzing for hangs. |
| Undefined behavior (CWE-758) | UndefinedBehaviorSanitizer runs in CI. |
| Vulnerable dependency (CWE-1395) | Three codec dependencies (libdeflate, OpenJPH, zstd), scanned by Snyk and vendored in `external/` at pinned release version SHA. |
| Tampering with releases or the build | Signed releases, GitHub Actions pinned by commit SHA, and reproducible builds (see "Reproducible Builds" in the website's install guide). |

The countermeasures are checked via CI:

- OSS-Fuzz continuous fuzzing, and the in-tree fuzz tests;
- CodeQL and SonarCloud static analysis, and Valgrind;
- AddressSanitizer and UndefinedBehaviorSanitizer CI jobs;
- code coverage measurement.

All critical software changes are reviewed and approved by at least one
maintainer before they are merged.

Each reported GitHub Security Advisory is fixed. Vulnerabilities that
meet the criteria in "CVE Assignment" above are assigned CVE identifiers and listed under
"Known Vulnerabilities" above, so the weakness classes found in
practice are tracked.

### Residual risks

The assurance case does not claim that OpenEXR is free of
vulnerabilities. The remaining risks are:

- Image and tile size limits are opt-in, so an application that does
  not set them can be exhausted of memory by a file that declares very
  large dimensions.
- Fuzz testing covers only small inputs, so it cannot rule out flaws
  that need large files to trigger.
- The codec libraries (libdeflate, OpenJPH, zstd) are trusted as part
  of the input path. The library checks the sizes they produce, but
  not their internal correctness.
- A single maintainer's approval is sufficient to merge a change, so
  review depends on that maintainer's diligence.

These risks are accepted to preserve compatibility with existing files
and to keep the project's review process lightweight, and they are
reduced by the countermeasures above.
