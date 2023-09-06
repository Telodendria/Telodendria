# Telodendria Matrix Specification Roadmap

This document provides a high-level overview of Telodendria's roadmap as it pertains to implementing the Matrix specification. Essentially, the Matrix specification is divided up into manageable portions amongst Telodendria releases, so that each release up until the first stable release implements a small portion of it.

**Note:** The first stable release of Telodendria will implement Matrix v1.7, no newer version. The Matrix specification changes too frequently, so I had to just pick a version in order to make this project manageable. Once v1.7 is complete, then we can move on to later specs.

This document will be updated to include more implementation details as they come up. It contains the big picture for far-out releases, and more relevant implementation details for near releases.

## Milestone v0.4.0

- [ ] Client-Server API
	- [ ] **7:** Events
		- [x] Compute size of JSON object in `CanonicalJson`
		- [x] Rename `Sha2.h` to just `Sha.h`; add `Sha1()` function
		- [x] Make `Sha256()` just return raw bytes; add function to convert to hex string.
	- [ ] **8:** Rooms
	- [ ] **9:** User Data
		- [x] Profiles
		- [ ] Directory

## Milestone v0.5.0

- [ ] Client-Server API
	- [ ] Modules
		- [ ] Content Repository

## Milestone v0.6.0

- [ ] Client-Server API
	- [ ] Modules
		- [ ] Instant Messaging
		- [ ] Voice over IP
		- [ ] Receipts
		- [ ] Fully Read Markers
		- [ ] Send-To-Device Messaging
	- [ ] Security (Rate Limiting)

## Milestone v0.7.0

- [ ] Server-Server API

## Milestone v0.8.0

- [ ] Application Service API
	- [ ] YAML parser?

## Milestone v0.9.0

- [ ] Identity Service API

## Milestone v1.7.0 (Stable Release!)

- [ ] Push Gateway API
- [ ] Room Versions
