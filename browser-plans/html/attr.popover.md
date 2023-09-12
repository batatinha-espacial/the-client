# HTML Global Attribute - popover

The popover global attribute is used to designate an element as a popover element.<br>
Popover elements are hidden via display: none until opened via an invoking/control element (i.e. a `<button>` or `<input type="button">` with a popovertarget attribute) or a HTMLElement.showPopover() call.<br>
When open, popover elements will appear above all other elements in the top layer, and won't be influenced by parent elements' position or overflow styling.
Possible values:
- `auto` or an empty string: can be "light dismissed" by selecting outside the popover area, and generally only allow one popover to be displayed on-screen at a time.
- `manual`: must always be explicitly hidden, but allow for use cases such as nested popovers in menus.