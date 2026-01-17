import { AnyRenderFunction } from 'valdi_core/src/AnyRenderFunction';

type GetTypeOfChildren<TViewModel> = TViewModel extends { children: any } ? TViewModel['children'] : never;

/**
 * Compiler annotation to mark that a render function should be provided
 * as the slot of a component through its "children" view model property.
 * This function can only be called in TSX.
 * @param value the render function that should be passed as the view model "children" property
 */
export declare function $slot<T extends AnyRenderFunction>(value: T | undefined): T | undefined;

/**
 * Compiler annotation to mark that an object should be treated as a named slots,
 * and provided to the component through its "children" view model property.
 * This function can only be called in TSX.
 * @param value the named slots object that should be passed as the view model "children" property.
 */
export declare function $namedSlots<TNamedSlots>(value: GetTypeOfChildren<TNamedSlots>): GetTypeOfChildren<TNamedSlots>;
