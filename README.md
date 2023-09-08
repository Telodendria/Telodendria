# [Telodendria](https://telodendria.io)

Telodendria is an extremely powerful, yet lightweight and portable
chat server designed to be easy to install and configure. Powered by
the [Matrix](https://matrix.org) protocol, Telodendria empowers
everyone to run their own chat server on ordinary hardware, including
old and embedded devices. Whether you want a simple chat server just
for you and your friends and family, or want to talk to users on other
Matrix homeservers but don't want to go through all the hastle of
hosting a complicated, high-maintenance homeserver or joining an
existing homeserver for privacy or other reasons, then Telodendria
might be for you.

## What is Matrix?

Matrix is an **open standard** for *interoperable*, *decentralized*,
*secure*, and *real-time* communication over the internet.

Matrix can be thought of as the successor to email, but it works
very similar to iMessage, Discord, or direct messaging on most
social media networks. The primary difference between Matrix and these
other services, however, is that Matrix doesn't depend on one central
authority, and is designed in such a way to respect your privacy.
Matrix has proven itself over the last few
years to be a reliable communication tool, and has only gotten more
user-friendly over the course of its development. Matrix is capable
enough that it can&mdash;and should&mdash;totally replace any other
means of digital communication, and it offers a much higher degree
of security, simplicity, and functionality.

Strictly speaking, Matrix itself is just the *protocol* by which
clients and servers communicate. In order to use Matrix, we need
implementations of both clients and servers. Telodendria is a server
implementation of the Matrix protocol.

## Why Telodendria?

- **Lightweight:** Written in the C programming language, Telodendria
is automatically lighter and faster than other self-hosted chat servers.
It has very few external dependencies and is as self-contained as
possible.
- **Fully-Featured:** Most lightweight chat solutions compromise on
features. Telodendria is built on the fully-featured Matrix protocol,
which provides a chat experience that most normal users are familiar
with.
- **Portable:** You can run Telodendria on just about everything,
including more traditional options like a personal home server or VPS,
but also more obscure platforms like Raspberry Pis or retro computers.
Telodendria can run on a broad number of operating systems, which means
that no matter which platform and OS you prefer, there is a good chance
you can add Telodendria without much difficulty. It is also extremely
easy to migrate a Telodendria instance between platforms; just copy the
data directory to a new device.
- **Simple:** Telodendria is designed to be a simple, no-frills
chat server. It is easy to install, easy to configure, and easy to
maintain.
- **Stable:** Other Matrix homeservers develop at the pace of the
Matrix specification itself, which is to say quite rapidly. Changes are
always being made, and a version shipped 6 months ago is already
incredibly outdated. Telodendria, on the other hand, aims to be stable.
It should *just work* for long periods of time between upgrades, and
you should never feel like Telodendria is going to change significantly
between upgrades.

[Read Technical Rationale &rightarrow;](docs/dev/rationale.md)

## Get Started

Check out the [Documentation](docs/README.md) to get started with
Telodendria.

## Status

## Sponsorship

Telodendria is maintained by a loosely-knit band of volunteers. The
project currently has no sponsors and thus no source of income to
pay for infrastructure costs and developer time. To ensure
Telodendria's long-term success, please consider sponsoring the
project.

You can make a recurring donation to Telodendria using
[LiberaPay](https://bancino.net/Telodendria/donate). You can also make
one-time donations using
[Stripe](https://donate.stripe.com/8wM29AfF5bRJc48eUU). If you would
like to make a recurring donation larger than that allowed by
LiberaPay, please contact Jordan Bancino over Matrix at
`@jordan:bancino.net` or email at `jordan@bancino.net`.

### Benefits

While there are no set sponsorship tiers at this time, sponsoring
Telodendria is a mutually beneficial relationship. Depending on the
amount you donate, you can get your name, logo, and website links
on the [Sponsors](docs/SPONSORS.md) page, the project `README`, or the
main website.

## License

All of the code and documentation for Telodendria is licensed under a
modified MIT license. The MIT license is an extremely permissive
license that has very few restrictions. Please consult the
[`LICENSE.txt`](LICENSE.txt) file for the actual license text. It is
important to note that the Telodendria license text differs from the
original MIT license in the following ways:

- Where the MIT license states that the copyright notice and permission
notice shall be included in all copies or *substantial* portions of the
software, the Telodendria requires the copyright notice and
permission notice be included with *all* portions, regardless of the
size, by omitting the word *substantial*.

The Telodendria logo in all its forms, including the ASCII
representation, belongs solely to the Telodendria project. It must be
used only to represent the official Telodendria project. You are free
to use the logo in any way as long as it represents or links to the
official project. If Telodendria is forked, the logo must be removed
completely from the project, and optionally replaced by a different
one.

