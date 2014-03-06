Jeremy Chen
7972623143

README FILE for Assignment 2 - Rollercoaster

To create my rollercoaster spline, I relied heavily on a Catmull-Rom equation using the function cm_equation, where it took in four points and multiplied it by the basis matrix and the equation u^3+u^2+u+1. I also called a draw function within display, which actually creates the spline by making the vertices necessary for GL_LINES.

The ground texture and sky texture were both created with GL_QUADS, and the sky texture needed to have six faces to create a skybox effect.

The ride, or camera, also used the cm_equation and derived the tangent, normal and binormal vectors (I also utilized the functions normal() which normalized a vector and cross_product() which found the cross product of two vectors). By interpolating multiple points, I was able to get the point that represents the rider's position and also the point that represents where the rider should be looking. After tweaking these values slightly to make it look nicer, I put it in gluLookAt to implement the camera.

Extra Features:
1. Personalized spline, denoted by myspline.sp. 
2. Spline is closed with C1 continuity
3. Lights are used
4. Restart function included when you right-click
5. Rollercoaster has two rails and is colored differently

Animation jpg files are located in the folder "Animation".

*Important Note*
My program is reliant on having two of the same spline files in track.txt. If two different ones are included, there will be two different splines viewed and it will look messy. If only one is included, there will be a segmentation fault because my program always calls a second spline in the spline array.