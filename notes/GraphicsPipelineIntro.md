This part makes a brief introduction for creating graphics pipeline. Graphics pipeline is a sequence of taking vertices and textures into pixels in render targets.

Sequence is like this:

**1. Vertex/index buffer**

**2. Input assembler**

    Collects raw vertex data from the buffers and may also use index buffer to avoid duplicating vertex data.

**3. Vertex shader**

    Run for every vertex. Applies transformations to turn vertex positions from model space to screen space. Also passes per-vertex data down the pipeline.

**4. Tessellation**

    Allow us to subdivide geometry based on certain rules to increase the mesh quality. Used to make brick walls and stairs look less flat while they are nearby.

**5. Geometry shader**

    It is run on every primitive and can discard it or output modere primitives than came in. It is similar but more flexible than tessellation. However, performance is not good.

**6. Rasterization**

    Discretizes the primitives into fragments. These are pixel elements that fill on the framebuffer.Any fragments that fall outside the screen are discarded and the attributes outputted by the vertex shader are interpolated across the fragments, as shown in the figure. Usually the fragments that are behind other primitive fragments are also discarded here because of depth testing.

**7. Fragment shader**

    It is invoked for every fragment that survices and determines which framebuffer(s) the fragments are written to and with which color and depth values. It can do this using the interpolated data from the vertex shader, which can include things like texture coordinates and normals for lighting.

**8. Color blending**

    Applies operations to mix different fragments that map to the same pixel in the framebuffer.Fragments can simply overwrite each other, add up or be mixed based upon transparency.

**9.  Framebuffer**



**Input assembler**, **Rasterization** and **Color blending** are known as **fixed-function** stages. These stages allow us to tweak their operations using parameters, but the way they work is predefined.

**Vertex shader**, **Tessellation**, **Geometry shader** and **Fragment shader** are programmable stages. We can upload out own code to the graphics card to apply exacly the operations we want. This allows us to use fragment shaders, for example, to implement anything from texturing and lighting to ray tracers. These programs run on many GPU cores simultaneously to process many objects, like vertices and fragments in parallel.

Graphics pipeline in **Vulkan** is immutable. In order to change it, we have to create a new one. This approach is harder to implement but it performs better since drivers optimize it much better.

Some programmable stages are optional. For example, tessellation and geometry stages can be disabled if we are just drawing simple geometry. If we are only interested in depth values then we can disable the fragment shaders stage, which is useful for **shadow map** generation.

For the next chapter, we are goint to implement vertex and fragment shaders to draw triangle onto the screen. The fixed function configuration like blending mode, viewport, rasterization will be set up in the chapter after that. The final part of setting up the graphics pipeline in **Vulkan** involves the specification of input and output framebuffers.

We create *createGraphicsPipeline* function that is called right after *createImageViews* in *initVulkan*. We'll work on this function throughout the following chapters.

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
}

...

void createGraphicsPipeline() {
}

```