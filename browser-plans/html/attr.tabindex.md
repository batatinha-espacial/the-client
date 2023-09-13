# HTML Global Attribute - tabindex

The tabindex global attribute allows developers to make HTML elements focusable, allow or prevent them from being sequentially focusable (usually with the Tab key, hence the name) and determine their relative ordering for sequential focus navigation.<br>
It's value is a number:
- A negative value means that the element is not reachable via sequential keyboard navigation.
- `0` means that the element should be focusable in sequential keyboard navigation, after any positive tabindex values. The focus navigation order of these elements is defined by their order in the document source.
- A positive value means the element should be focusable in sequential keyboard navigation, with its order defined by the value of the number.