# Element References

## IRenderedElement

The `IRenderedElement` interface is an abstraction over a rendered element. It allows you to see which attributes were applied, set new attributes, get the children elements, get the parent element, get a reference to the component responsible for rendering it. One could get an `IRenderedElement` instance for the root node and introspect/traverse the entire element tree at runtime.

Note the `ref` attribute. This attribute allows you to pass an `ElementRef` instance to a TSX object. The rendered node will be inserted inside the given ElementRef. 

## ElementRef

An instance of this class can be passed through the ref attribute to capture rendered elements. It also has a `setAttribute()` method, which can be used to set attributes on all held elements at once, as well as to any new elements which might be rendered later on. Example of use:

```tsx
interface ViewModel {
  elementSize: number
}
export class MySlotComponent extends Component<ViewModel> {
 elementsInSlot = new ElementRef();
 
 onCreate() {
  // This will set the backgroundColor to orange for any elements
  // which renders in our slot.
  this.slotRef.setAttribute('backgroundColor', 'orange');
 }
 
 onRender() {
  <view height={'100%'} alignItems='center'>
    <slot ref={this.elementsInSlot} />;
  </view>

  // This will update the size of every elements in our slot
  // based on a view model property
  this.elementsInSlot.setAttribute('width', this.viewModel.elementSize);
  this.elementsInSlot.setAttribute('height', this.viewModel.elementSize);
 }
}
```
