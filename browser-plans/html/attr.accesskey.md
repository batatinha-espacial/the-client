# HTML Global Attribute - accesskey

The access key global attribute provides a hint for enerating a keyboard shortcut for the current element.<br>
The attribute value must be a single printable caracter, or multiple printable characters separated by space.<br>
If there's only one printable character, the browser will use that printable character.<br>
If there's only one printable character and it's not in the user's keyboard layout, the whole attribute is ignored (but it's not removed, just ignored).<br>
If there are multiple printable characters, the first one that is in the user's keyboard layout is choosed. If none of them are in the user's keyboard layout, again the whole attribute is ignored.

Our browser offers two keyboard shortcuts for accessing elements with this attribute:
- Alt + key
- Alt + Shift + key