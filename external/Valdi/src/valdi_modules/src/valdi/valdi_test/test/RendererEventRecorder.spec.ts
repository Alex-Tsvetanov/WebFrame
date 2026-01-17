import 'jasmine/src/jasmine';
import { RendererEventRecorder } from 'valdi_core/src/debugging/RendererEventRecorder';

describe('RendererTracerRecorder', () => {
  it('can print to string', () => {
    const recorder = new RendererEventRecorder();
    recorder.onRenderBegin();
    recorder.onComponentBegin('root', { name: 'RootComponent' });
    recorder.onComponentBegin('child', { name: 'ChildComponent' });
    recorder.onComponentEnd();
    recorder.onComponentEnd();
    recorder.onRenderEnd();

    expect(recorder.toString().trim()).toEqual(
      `
Begin render
  Begin RootComponent (key: root)
    Begin ChildComponent (key: child)
    End ChildComponent
  End RootComponent
End render
    `.trim(),
    );
  });

  it('can automatically shrink on render end', () => {
    const recorder = new RendererEventRecorder();

    recorder.setMaxBufferSize(8);
    recorder.onRenderBegin();
    recorder.onComponentBegin('root', { name: 'RootComponent' });
    recorder.onComponentBegin('child', { name: 'ChildComponent' });
    recorder.onBypassComponentRender();
    recorder.onComponentEnd();
    recorder.onComponentEnd();
    recorder.onRenderEnd();

    expect(recorder.toString().trim()).toEqual(
      `
    Begin ChildComponent (key: child)
      Bypass render
    End ChildComponent
  End <unknown>
End render
    `.trim(),
    );
  });
});
