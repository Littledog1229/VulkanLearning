## My Original Introduction to Vulkan's Hello-Triangle

This archived repository is home to my first attempt at using the Vulkan Graphics API (Vulkan 1.0, speciifcally), and really introduced me to the complexities other graphics frameworks
(primarily OpenGL) tend to hide from the average developer. Not only did I learn that there was a lot more that I had to handle myself (I mean, manually selecting what type of GPU
memory your vertex buffer uses was never a consideration I had to make when using OpenGL), but it really introduced me to the power you can acquire when going further down to 
bare-metal when working with GPUs. Synchronization primitives like Semaphores and Fences were completely brand new to me, and the manual creation of the swapchain wasn't really
something I never really had to deal with before (in fact, I barely knew about the swapchain to begin with).

Vulkan is an interesting beast, and though it can be challenging to use it definitely seems to open up a whole world of possibilities (the specific one I'm interested in is
the Raytracing extension and shaders, but that's something I have never touched before so it would be a journey). Although most projects I work on don't really need the sheer
power it provides (many of the 2D applications I started for fun [and never finished] would never need this level of performance and control), It's still be a fun journey
trying to learn the ins-and-outs of how modern GPUs work under the hood.

One day I plan on getting decent at using Vulkan (in fact, im using Silk DotNet right now for C#-based Vulkan experimentation [and have an almost-complete Hello Triangle port]),
but right now I have other priorities I need to focus on. Although thousand-line triangle can be tough, it's definitely something worth experiencing to really get an appreciation
of what most graphics libraries hide away from you. The possibilities seem endless, and one day I hope to be able to fully utilize the potential of Vulkan (hopefully not 1.0, the
dynamic rendering of 1.3 seems a lot better when getting started [and if I recall things like Synchronization2 are just better]).

 -- ZQM.
