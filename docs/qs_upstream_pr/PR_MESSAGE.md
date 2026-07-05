## Title

Add CITATION.cff (enables GitHub's "Cite this repository" button)

## Body

Hi Michel,

Thank you for C-Quadratic-Sieve. I've used it for years in a
number-theory hobby project (factoring candidate values on the curve
2n²+2n+1), and it's been rock-solid — self-contained, fast, and the
factor output is trivially self-verifiable. Much appreciated.

This PR adds a small `CITATION.cff` file. Its only effect is to enable
GitHub's built-in **"Cite this repository"** button on your repo's front
page, which generates a ready-made citation (APA, BibTeX, etc.) for
anyone who wants to credit the tool. It's metadata only — it changes no
code and imposes no license or obligation.

I pulled the fields (title, v2.0.0, 2025-02-01, description) from your
README; please correct anything I got wrong — especially if you'd rather
list your name differently or add an ORCID.

I deliberately left out a `license:` field, since the README dedicates
the work to the public domain without a formal license file and I didn't
want to put words in your mouth. If you ever want the citation/license
metadata to be fully machine-readable, an SPDX id such as `Unlicense`
or `CC0-1.0` (plus a matching LICENSE file) would do it — but that's
entirely your call, and this PR works fine without it.

No worries at all if you'd prefer to keep the repo as-is — feel free to
close this. Thanks again for a great tool.
