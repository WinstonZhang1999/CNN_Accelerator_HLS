USER NAME: wjz2102  
HOST NAME: socp03  
LAYER: 1  
PROJECT ID: 1l
Project
CSEE 4868: SoC Platforms November 18th, 2021
IMPORTANT: To get full credit:
• do not modify the directory structure and the file names in the GIT repository,
• make sure the code runs on the socp0X.cs.columbia.edu servers,
• to build and run your code source “/opt/cad/scripts/tools env.sh”;
• DO NOT commit binaries, output files or log files;
• DO NOT commit modifications to any Makefile, test.sh and team.tcl.
You will carry out the project individually. A repository is assigned to each student and can be cloned by executing:
                git clone git@soc.cs.columbia.edu:socp2021/prj-UNI.git
1 Convolutional Layers
Recall the DWARF-7 convolutional neural network (CNN) from the homework assignments (Figure 1). In this project, the focus moves from the fully connected layers from Homework 4 back to the convolutional layers from Homework 2, for which we provide a baseline implementation of a hardware accelerator. Your task is to perform a design space exploration (DSE) on the provided accelerator.
We suggest to carefully study the convolution compute() function of the Mojo application and see how the convolution is implemented. Adding prints in the programmer’s view can help you find out the values of the parameters passed to each layer.
Furthermore, understanding the general idea a convolutional layer is important. In addition to the lecture slides, the following are some recommended references on CNNs:
2
• MaterialfromStanford’sSpring2017class[1]CS231n:ConvolutionalNeuralNetworksforVisualRecognition.
– Notes (highly recommended): http://cs231n.github.io/convolutional-networks/
– Slides: http://cs231n.stanford.edu/slides/2017/cs231n 2017 lecture5.pdf
– Lecture video: https://www.youtube.com/watch?v=bNb2fEVKeEo&feature=youtu.be
• Beginner’s overview: part 1 [2] and part 2 [3]. Baseline Micro-Architecture
As known from previous homework assignments and shown in Figure 1, the DWARF-7 network is divided into 6 layers. The system can process data in a streaming fashion because it can work as a pipeline at the granularity of a layer. For example while Layer 2 is working on the inference of a dog image, Layer 1 could be working on the image of a cat and Layer 3 on one of a automobile.
Each student is assigned one of the four convolutional layers. We provide a baseline accelerator implementation that works for all the layers. The information on which layer is assigned to you is taken from the team.tcl file. The infrastructure is the same as that of Homework #4 with the addition of the convolutional layer implementation in SystemC.
The provided accelerator merges together accumulation, activation, padding (also called resize) and pooling. Notice that only some convolutional layers in DWARF-7 require all of the above.
1

 Figure 1: DWARF-7 system for the project with the layers. The design-space exploration will be performed both at component and system level. The performance metrics for the exploration are shown in the two charts.
3 Design Space Exploration
The main goal of the project is to perform a competitive and a collaborative design space exploration for the compo- nents and the system shown in Fig. 1. In this context, the component is one particular convolutional layer, while the system is the overall DWARF-7 CNN. At the component level, you should start from the baseline micro-architecture that we provide and explore a wide variety techniques for design space exploration with the help of high-level synthesis (HLS). We purposely provide a sub-optimal baseline that has a lot of room for improvement.
You are required to obtain at least 3 Pareto-optimal micro-architectures that are named FAST, MEDIUM, and SMALL, respectively. These have been specified as targets for the HLS in the file project.tcl, where you can add synthesis attributes to each hls config. Even though you should design 3 different micro-architectures, you are asked to commit a single SystemC module where C preprocessor directives are used to switch between the different implementations depending on the target:
#if defined(SMALL)
// Source code specific to the SMALL target...
#elif defined(MEDIUM)
// Source code specific to the MEDIUM target...
#elif defined(FAST)
// Source code specific to the FAST target...
#endif
Throughout the project you should keep improving your own Pareto frontier by either advancing your Pareto optimal designs or by adding new ones. Eventually, you should aim at having your designs on the Pareto curve of the class for the specific convolutional layer assigned to you. Daily, starting from Monday November 22nd at 11.59 pm, we are going to automatically test your submitted designs. We will consider the last git commit of the day. The performance of the valid designs will be published daily on the project website: http://pelican.cs.columbia.edu:3838/socp2021/
2

The website allows you to observe the anonymized results of the other students. On certain dates (see Section 4) you will be able to reuse the work of the other students in the class in order to combine the design of your components with theirs and build the full DWARF-7 system. Similarly, the other students will be able to reuse your design. In this way the project promotes design for reusability.
For each of your micro-architectures you have to select 5 implementations, one from each of the layers other than yours. This means that you are going to select 5 implementations to be combined with your SMALL micro-architecture, 5 to be combined with your MEDIUM micro-architecture, and 5 to be combined with your FAST implementation. You should carry out the selection of the implementations on the website and then download the updated header file precision test fpdata.hpp, which is automatically generated. You must copy this file into your repository in the folder hls/tb/. Remember to push this file with git when you do your submission. The selection feature for the system composition on the website will become available later on, closer to the first system composition dead- line (see Section 4). NOTE. You should not edit the downloaded file. The data types specified there are used for a system-level simulation that measures the accuracy of the system implementation. You may run the system accuracy test yourself locally on your socp0X server (see the make target in Section 5.1). The system accuracy simulation is available in a folder called accuracy in your repository.
In your repository you will find your ID in hls/syn/team.tcl. On the website we will use your ID to label your designs. Since students will compete for some parts of the project, you are discouraged from sharing your ID with others.
While initially your choice of is limited, there will be more opportunities to compose a better system, as every student makes progress. For each micro-architecture that you choose for each of the other five layers, you must specify how many instances you want to use for that layer; this choice will greatly impact area and throughput. After you push your system choices into the git repository, the performance of your system will appear on the website the next day. Ultimately, these are the main goals:
• Component level: Try to improve the design of the accelerator for your convolution layer daily. Compete with the students that have been assigned your same layer by trying to optimize your design such that it lies on the Pareto curve with respect to the component-level metrics. This will increase the likelihood that your component design is reused by other students.
3.1
3.1.1
Performance metrics Component Level
• Systemlevel:Combineyouracceleratordesignwiththoseoftheacceleratorsfortheotherlayersmadeavailable by the other students to build the full system. This instance of collaborative design will allow you to quickly obtain an implementation of the whole system. Then, compete with all the students by trying to refine your choice of components to obtain an implementation of the system that lies on the Pareto curve with respect to the system-level metrics.
At component level there are two metrics: area and effective latency. They are evaluated on a single instance of
your accelerator, which is then used to execute sequentially all the layers within your assigned layer. Each of your valid micro-architectures will appear as a point in a Component DSE chart on the website as shown Figure 2. A micro-architecture is valid if it meets the accuracy constraint explained below.
Accuracy and correctness. There are three requirements for an implementation to be valid:
• Functional correctness. The results of the behavioral SystemC simulation with float data types must
match exactly the results of the programmer’s view.
• HLS-generated RTL correctness. The results of the behavioral SystemC simulation with fixed-point data types must match the results of the RTL-SystemC co-simulation of the HLS-generated Verilog.
• Accuracy. The component accuracy must be at least 50% for the design to be valid. The accuracy test exe- cutes inference on 10 images and at least 5 have to be classified correctly. The test executes the behavioral simulation with your chosen fixed-point precision for the layer and the original floating-point precision for the other layers. See the Makefile target for this test in Section 5.1.
Area. The area of your accelerator is calculated in terms of FPGA-resource utilization on the target Xilinx zcu106 FPGA board. Specifically, the area is calculated based on the average of the percentage utilization of the main
3

Component DSE FAST
MEDIUM
SMALL
Latency
a)
System DSE
accuracy
0.91
0.85
0.98
0.90 0.87
Throughput
b)
  • With one instance per layer, the throughput is the inverse of the effective latency of the slowest layer: Tlayeri = 1
Tlayeri = 1 = #instances (Llayeri /#instances) Llayeri
System Level
At the system level there are three metrics: area, throughput and accuracy. Each of your valid system implementation is represented by a point on a System DSE chart as shown in Figure 2, where accuracy, the third metric, appears as a label next to each point.
Accuracy The accuracy is obtained by counting the percentage of input images classified correctly by the system composed by the chosen accelerators. There is no way to know in advance how the composition of modules will affect the overall accuracy, but we provide a Makefile target for this test in Section 5.1.
Area The area of the resulting system is computed as
ASystem = Alayer1 + Alayer2 + Alayer3 + Alayer4 + Alayer5 + Alayer6.
0.75
  Figure 2: a) Example of component-level Pareto curve. Here the three darker dots are Pareto optimal. b) Example of system-level Pareto curve. Notice that the micro-architecture labeled with 0.98 accuracy is Pareto optimal because no other design has better accuracy.
FPGA resources: BRAM, LUTs, and DSPs (Mults).
Alayer = (%BRAM + %LUTs + %DSPs)/3.
Be aware that by increasing the number of ports and access patterns of the private local memory (PLM) of the accelerator the number of BRAMs is going to grow significantly. Note that you cannot exceed the resources available on the FPGA.
Effective Latency. The effective latency LEff (ns) of a layer corresponds to the amount of time needed to process completely a given input image across the layer. The effective latency of a layer is reported at the end of the execution of the RTL simulation.
Throughput. The throughput is only used as a metric at the system level, but it needs to be defined at the component level as well. The throughput depends on the number of instances of an accelerator that are used for a given layer; this number may vary between one and the maximum of three.
 (Llayeri )
• If you select multiple accelerator instances for a specific layer, the throughput will be:
  3.1.2
4
Area
Area

The layer area depends on how many instances of an accelerator the designer chooses to instantiate: Alayer = Aaccelerator · Ninstances
Throughput Since the system is pipelined with the granularity of a layer, the effective throughput of the resulting system is computed as
Tsystem = min{Tlayer1, Tlayer2, Tlayer3, Tlayer4, Tlayer5, Tlayer6}.
4 Evaluation
4.1 Required Work (600 Points)
By the project deadline of December 21st at 11.59pm, you are required to submit the design of three micro-architectures of the accelerator for the assigned layer and three system-level implementations obtained through design reuse of the other layers, as explained in Section 3. Your designs will be evaluated based on the results of the whole class as follows:
•
•
4.2
125 points for each micro-architecture (SMALL, MEDIUM, FAST) for which you submitted a Pareto optimal design at component level. Students with no Pareto optimal design will get a score proportional to the distance of their design that is closest to the Pareto curve.
75 points for each micro-architecture (SMALL, MEDIUM, FAST) for which you submitted a Pareto optimal design at system level. Students with no Pareto optimal design will get a score proportional to the distance of their design that is closest to the Pareto curve.
Optional Work for Extra Credit (Up to 225 Points)
Every day from November 22nd to December 21st, a script will automatically evaluate all the designs that have been submitted on or before 11.59pm. The new valid designs will be automatically plotted on the charts on the website. This offers the opportunity for optional work that is highly encouraged and will be evaluated as follows:
• Daily self-progress: 150 points. 5 points per day if you improve your own Pareto frontier at the component level (Nov. 22nd to Dec. 21st included, that is 30 days) and submit your design by 11.59 pm. Your own Pareto frontier is considered improved if at least one among your new designs of the layer assigned to you is better than an older design by at least 5% either in terms of area or effective latency.
• Pareto optimal at component level on Nov. 29th, Dec. 6th, 13rd: 12 points per date. On each of these three dates the points are assigned if at least one of your design is Pareto optimal at the component level with respect to those of the other students working on the same layer. For each date, these points are assigned based on all the designs submitted on or before 11.59pm.
• Pareto optimal at system level on Nov. 30th, Dec. 7th, 14th: 13 points for date. On each of these three dates the points are assigned if at least one of your system composition is Pareto optimal at the system level with respect to those of all the other students. For each date, these points are assigned based on all the designs submitted on or before 11.59pm.
NOTE: Each design should pass the validation in order to be considered for credits. Designs that do not pass the validation will not receive credits and will not be plotted on the website.
5

5 Suggestions for Design Space Exploration
Here are some suggestions of micro-architectural choices or synthesis options for the design space exploration.
• Refactor the source code: There may be opportunities for optimization that you can explore by modifying the source code with respect to the provided baseline. The functions and the overall structure of the accelerator can be re-arranged or re-implemented in a variety of ways. To explore these different options, it may be helpful to allow these implementations to live side by side within the same shared source file. A recommended approach for doing this is to use C preprocessor directives to switch between the different source code implementations.
• Modify HLS attributes:
– Target clock period: Stratus HLS uses the clock period to characterize datapath parts and to schedule parts into clock cycles. The number of operations scheduled in a clock cycle depends on the length of the clock period and the time it takes for the operation to complete. If the clock period is longer, more operations are completed within a single clock period. Conversely, if the clock period is shorter (higher clock frequency), Stratus HLS automatically schedules the operations over more clock cycles. Please, note that for some values of clock period Stratus HLS may not be able to instantiate big arithmetic operators. You can specify a different clock period for each micro-architecture in the file project.tcl.
• Modify fixed-point precision: You can modify the precision of the fixed-point data types by changing the number of bits used for the representation. You can do so by editing the constants in datatypes.hpp. Notice that two different fixed-point types are defined there: W FPDATA is meant to be used for the weights, FPDATA for everything else. A data type with less bits requires much smaller and faster operators (addition, multiplication) and less local memory space. Therefore, a reduced precision improves both area and latency, but it might deteriorate the accuracy.
Reducing the fixed-point precision with respect to the software execution means that your accelerator is discard- ing some bits of the original 32-bits data words. In this case you could reduce the precision already in the user application in order to store less accelerator’s data in memory. In this way the accelerator loads less bytes of data overall. A similar approach applies to the accelerator’s output.
• Apply HLS knobs:
– Loop unroll: The loop unrolling transformation duplicates the body of the loop multiple times to expose additional parallelism that may be available across loop iterations. You can completely unroll, partially unroll or not unroll at all. Remember that higher parallelism means better performance but more area.
– Loop pipeline: Loop pipelining enables one iteration of a loop to begin before the previous iteration has completed, resulting in a higher throughput design while still enabling resource sharing to minimize the required area. This makes it possible to incrementally trade off improved throughput against potentially increased area.
– HLS CONSTRAIN LATENCY: The directive is used to specify the minimum and maximum acceptable latency for a block of code. The block can be a loop body, an if/else or switch branch, or straight line code enclosed by braces. See the manual for more details and examples.
– HLS SCHEDULE REGION: The directive provides a way to have Stratus HLS segregate part of a de- sign’s functionality for separate processing with minimal modification to the source code structure. The operations in the region are synthesized as if they were in a separate SC THREAD, producing an inde- pendent finite state machine in RTL. This FSM can implement the operations with a fixed latency, with a variable latency, or in a pipelined manner. See the manual of Stratus HLS for more details and examples.
– HLS CONSTRAIN REGION: The directive can be used to control the latency and timing of the re- source created for an HLS SCHEDULED REGION. This optional directive should be placed adjacent to a HLS CONSTRAIN REGION directive. See the manual for more details and examples.
– Stratus HLS offers many more “knobs” to interact with. The User Manual, the Reference Guide, and the class slides provide further documentation. We suggest to try them and to keep monitoring the web forum for additional hints.
• Customize the private local memory (PLM): You have complete freedom on the organization and the size of the PLM, as long as it fits on the target FPGA.
6

•
5.1
Customize the DMA channels: It is possible to both change the bitwidth of the DMA or the number of DMA channels. Notice that the AXI bus would have to reflect these changes. The biggest effect of this kind of optimization is the accelerator’s bandwidth to memory. To apply these modification you should act on the memory model in the SystemC testbench. Notice that the minimum data width on the AXI bus is 32 bits. You are highly encouraged to at least increase the DMA bitwidth, which is expected to provide great speedups.
Make Targets and Design Flow
In this section we describe all the useful Makefile targets that we provide. In the targets, <cfg> can be either FAST, MEDIUM or SMALL, <layer#> is the number of the layer to be simulated and <image> can be any of the images in the data folder.
You should run the targets from the hls/syn/ directory.
• $ make Makefile.prj
Generation of HLS dependencies, including the memory models listed in memlist.txt. You do not need to run this target, it is run as a dependency of other targets.
• $ make sim BEHAV NATIVE <cfg> TARGET LAYER <layer#> <image>
Behavioral simulation with native floating-point numbers. This simulation is faster than the one with fixed point numbers. Use this target to test and debug the functionality of your SystemC accelerator. The results for this target are stored in: test.txt, they have to match those generated by the programmer’s view for that specific layer and image.
• $ make sim BEHAV ACCELERATED NATIVE <cfg> TARGET LAYER <layer#> <image> Accelerated version of the behavioral simulation with native floating-point numbers. Use this target to test and debug the functionality of your SystemC accelerator. This target is accelerated in that it does not execute the layer in full, it only executes on a limited amount of source and destination channels. The accelerated results for this target are stored in: accelerated test.txt. When running this target standalone, you should rename the output txt file and store it outside the project synthesis folder to keep it as a reference later on
• $ make sim BEHAV <cfg> TARGET LAYER <layer#> <image>
Behavioral simulation with SystemC fixed-point numbers. The results for this target are stored in: test.txt. Similarly to the native simulation, consider renaming and moving the output txt file.
• $ make sim BEHAV ACCELERATED <cfg> TARGET LAYER <layer#> <image>
Behavioral simulation with SystemC fixed-point numbers. Since the Verilog simulation is too slow, we provide this ACCELERATED target to compare against an accelerated Verilog simulations. It executes only part of the inference and saves the partial output in accelerated test.txt. Similarly to the native simulation, consider renaming and moving the output txt file.
• $ make hls <cfg>
Perform HLS for one of the micro-architectures (i.e. one hls config).
• $ make sim <cfg> ACCELERATED TARGET LAYER <layer#> <image> V
Simulation of the Verilog accelerator generated by Stratus HLS. The regular Verilog simulation would be too slow, so we provide this ACCELERATED target. The partial output is stored in accelerated test.txt. These results must match those of the accelerated fixed point behavioral simulation. The make hls <cfg> target is a dependency of this target. Again, you may want to rename and move the txt file.
• $ make debug <cfg> ACCELERATED TARGET LAYER <layer#> <image> V
This target is the same as above, but it opens the GUI for you, so you can debug more easily by looking at the waveforms.
• $ TEST LAYER=TARGET LAYER <layer#> make test <cfg>
This script executes the test.sh script, which runs the programmer’s view, the regular native simulation, the accelerated behavioral fixed-point simulation and accelerated Verilog simulation. All of the simulations run on the cat image. It first runs the programmer’s view, then runs the native behavioral simulation and compares the results. Then it executes both accelerated fixed point simulations, behavioral and RTL, and it compares their results. We will use this target to test the correctness of your submitted designs.
7

6
•
$ make accuracy component <cfg>
Classification test on all the images by using a DWARF-7 where your layer executed with your choice of fixed point precision and the other layers are executed in their original floating point precision. This target evaluates the component-level accuracy, which has to be at least 50% for the design to be valid. This test is done by executing the native simulation. We will use this target to test the component-level accuracy of your submitted designs.
$ make accuracy system <cfg>
Classification test on all the images by using a DWARF-7 built with you layer choices on the website. This target evaluates the system accuracy, which will appear in the system-level plot on the website. This test is done by executing a modified C-language version of the original Mojo programmer’s view. We will use this target to test the system-level accuracy of your submitted designs.
$ IMAGE=<image> TARGET LAYER=<layer> make dwarf-run
This target is the only one that’s executed only under the programmer’s view folder (pv) and not under hls/syn. This target executes the programmer’s view code and saves the layer’s output to test.txt.
Private Local Memory (PLM) Generation
•
•
The target make Makefile.prj generates Stratus HLS scripts and its dependencies, including the definition and the Verilog models of the private local memory (PLM). The PLM is generated based on the specification given in the file memlist.txt. Each line of this file describes a memory to be generated with the following syntax:
<name> <words> <width> <parallel op list> • <name>: Memory name;
• <words>: Number of logic words in memory;
• <width>: Word bit-width;
• <parallel op list>: List of parallel accesses to memory. These may require one or more ports.
The <parallel op list> contains all the possible access patterns that the PLM can support. An access pat- tern is described by the number of read and write operations that can be executed simultaneously. Each element of <parallel op list> has the following syntax: <write pattern>:<read pattern>.
Let’s consider an accelerator that uses a PLM called plm0 with size of 1024 words of 32 bits each; let’s also assume that the accelerator access the PLM only in 3 possible scenarios: (a) read 8 contiguous words simultaneously; (b) read only 1 word; and (c) write only 1 word. A memory with the above specifications should be generated with the following description: plm0 1024 32 1w:0r 0w:8r 0w:1r.
The previous example corresponds to the case when the 8 words that are read are stored in contiguous locations in the PLM. Instead, if multiple words are read in locations that are not contiguous, then the corresponding access pattern must be specified as 0w:8ru, where a u is appended after r (or after w in the case of writing).
Lastly, it is possible that reads and writes happen simultaneously on the same PLM. If for example 1 word is read and 1 other word is written at the same time, the access pattern would be: 1w:1r.
7 Resources
Since Stratus HLS is a powerful industrial CAD tool, you may find it helpful to refer to its documentation in addition to the in-class tutorials we gave on its use. You may find the documentation in /opt/cad/stratus/doc/ on the socp0* servers. The two PDFs you are likely to find most helpful are:
• /opt/cad/stratus/doc/Stratus HLS User Guide/Stratus HLS User Guide.pdf
• /opt/cad/stratus/doc/Stratus HLS Reference Guide/Stratus HLS Reference Guide.pdf You may download it to your local machine using rsync, scp or similar file transfer methods for easier viewing.
8

References
[1] https://www.cs.toronto.edu/ ̃frossard/post/vgg16/
[2] https://adeshpande3.github.io/adeshpande3.github.io/A-Beginner%27s-Guide-To-Understanding-Convolutional- Neural-Networks/
[3] https://adeshpande3.github.io/adeshpande3.github.io/A-Beginner%27s-Guide-To-Understanding-Convolutional- Neural-Networks-Part-2/
9

