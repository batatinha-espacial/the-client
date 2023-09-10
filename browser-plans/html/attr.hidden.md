# HTML Global Attribute - hidden

The hidden attribute is used to indicate that the content of an element should not be presented to the user.<br>
It's possible values are:
- `hidden` or an empty string: sets the element to `hidden` state.
- `until-found`: sets the element to `hidden until found` state.
- other values: sets the element to `hidden` state.

## The `hidden` state

Indicates that the element will not appear to the user in the page nor in the page layout.

## The `hidden until found` state

Indicates that the element will not appear on the page, but will appear in the page layout.<br>
When something causes the browser to scroll to that element, the browser will:
- fire a `beforematch` event on the element
- remove the hidden attribute
- scroll to that element