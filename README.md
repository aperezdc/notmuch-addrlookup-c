# Notmuch Address Lookup tool — in C

This is an address lookup tool similar to the `addrlookup` one found
in the (apparently unmaintained) [Vala Notmuch
binding](https://github.com/spaetz/vala-notmuch), using the
[Notmuch](http://notmuchmail.org) database as source.

The one and only feature is accepting a string as first argument, and it
will use the Notmuch database to output the names and e-mail addresses of
the people you have exchanged e-mails with, most used ones first. This
works neatly as [address completer for the Notmuch Emacs
interface](http://notmuchmail.org/emacstips/#index15h2).

One more thing: I plan to keep the tool up-to-date when new Notmuch
releases change the API of the library. Precisely the reasons why
this small project was started are:

* Lack of maintenance of the Vala version.
* The Python version included with Notmuch can be a tad slow when
  working with big amounts of messages (over tens of thousands).
* The [Go](http://golang.org) implementation included with Notmuch
  insists in outputting all the text in lowercase. Plus, requires
  a working Go development setup to build it.


## Building

Make sure you have the following installed, plus their development
packages:

* `libnotmuch` — Included with Notmuch.
* GLib — It is a dependency of Notmuch as well.
* `pkg-config`
* GNU Make and a working C compiler.

Then just build and install it:
```sh
make
sudo cp notmuch-addrlookup /usr/local/bin
```

## Using

You can check that the program works by invoking from a shell. The
only argument it takes is a string, which is searched for in the
e-mail addresses contained in the headers of the messages indexed
by Notmuch. Example output (names and addresses are not real):

```
% notmuch-addrlookup peter
Peter Jones <peter@jonesfamily.com>
John Peters <john.peters@gmail.com>
Peter Tosh <gimmesome@reggaemusic.net>
Noren Petersen <norenp@no.se>
%
```

### Emacs UI

The Emacs Notmuch UI
[can be configured](http://notmuchmail.org/emacstips/#index15h2)
to use the tool for completing addresses when composing.

### Mutt

The [Mutt UA](http://www.mutt.org/) can be configured to use
`notmuch-addrlookup` by setting the following options:

```
set query_command="notmuch-addrlookup --mutt '%s'"
```

### alot

The [alot](https://github.com/pazz/alot) MUA can be configured to use
`notmuch-addrlookup`, using an `[accounts]` section in the configuration
file similar to this:

```
[accounts]
  [[youraccountname]]
    realname = Your Name
    address = your@address.com
    ...
    [[[abook]]]
      type = shellcommand
      regexp = '^(?P<email>[^@]+@[^\t]+)\t+(?P<name>[^\t]+)'
      command = notmuch-addrlookup --mutt
      ignorecase = True
```


## License

This software is distributed under the terms of the [MIT
license](http://opensource.org/licenses/MIT):

```
Copyright (c) 2014 Adrian Perez de Castro <aperez@igalia.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

```
