# Navigation

## Introduction

The `valdi_navigation` module makes it possible to show and dismiss pages in an application. It supports presentation using horizontal and vertical animations. It follows the iOS model. The module is a work in progress.

## Usage

To use the navigation module, an application or a page should render a `NavigationRoot` component, which will expose a `NavigationController` allowing consumer to perform various navigation operations. The `NavigationRoot` passes the `NavigationController` instance as part of its slot:

```tsx
onRender(): void {
<NavigationRoot>
    {$slot(navigationController => {
        // Render the root page with the navigationController
    })}
</NavigationRoot>;
}
```

The navigation controller exposes the following key methods:

- `push()`: takes as parameter the component constructor, the view model and component context, and renders the component as a page with a horizontal animation
- `pop()`: Inverse operation of `push()`, will remove the last page from the stack with a horizontal animation
- `present()`: Takes as parameter the component constructor, the view model and component context, and renders the component as a page with a vertical animation
- `dismiss()`: Inverse operation of `present()`, will remove all the pages up to and include the last page that was presented.

Each sub pages in an application should inherit the `NavigationPageComponent` or `NavigationPageStatefulComponent` to get access to a `NavigationController` so that they can perform navigation operations on their own. Only pages rendered inside a `NavigationRoot` should inherit `NavigationPageComponent` or `NavigationPageStatefulComponent`. For example:

```tsx
@NavigationPage(module)
export class Page1 extends NavigationPageComponent<{}> {
  onRender(): void {
    <label font={systemBoldFont(17)} value="This is Page1" />;
  }
}
```

The `Page1` component can be opened as a page shown vertically or horizontally from any component that has access to a `navigationController`, which includes the component rendering the `NavigationRoot` as well as all `NavigationPageComponent` and `NavigationPageStatefulComponent` subclasses:

```tsx
// Push Page1 with a horizontal animation
navigationController.push({
    Page1,
    {}, // Specify the view model to pass to Page1
    {}, // Specify the component context to pass  to Page1
})

// Push Page1 with a vertical animation
navigationController.present({
    Page1,
    {}, // Specify the view model to pass to Page1
    {}, // Specify the component context to pass  to Page1
})
```

## Complete example

```tsx
export class App extends Component {
  private navigationController?: NavigationController;

  onRender(): void {
    <NavigationRoot>
      {$slot(navigationController => {
        this.navigationController = navigationController;
        <view backgroundColor="white" width="100%" height="100%" alignItems="center" justifyContent="center">
          {this.renderButtons()}
        </view>;
      })}
    </NavigationRoot>;
  }

  private renderButtons() {
    <CoreButton text="Page 1" onTap={this.handleOpenPage1} />;
    <CoreButton text="Page 2" onTap={this.handleOpenPage2} />;
  }

  private handleOpenPage1 = () => {
    this.navigationController?.push(Page1, {}, {});
  };

  private handleOpenPage2 = () => {
    this.navigationController?.push(Page2, {}, {});
  };
}

@NavigationPage(module)
export class Page1 extends NavigationPageComponent<{}> {
  onRender() {
    <CoreButton text="Back" onTap={this.handleBack} />;
  }

  private handleBack = () => {
    this.navigationController.pop();
  };
}

@NavigationPage(module)
export class Page2 extends NavigationPageComponent<{}> {
  onRender() {
    <CoreButton text="Back" onTap={this.handleBack} />;
  }

  private handleBack = () => {
    this.navigationController.pop();
  };
}
```
