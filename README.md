# ConvexHull
Implementing Graham Scan to calculate the Convex Hull of a set of points.

30 points:
![image](https://github.com/user-attachments/assets/9904c446-3cc6-421f-a9c5-e4e01b555b43)

300 points:
![image](https://github.com/user-attachments/assets/fd5a8d9e-6908-4519-91d9-ec268768757c)

3000 points:
![image](https://github.com/user-attachments/assets/0ed41ae2-7167-4e87-b6dd-6aaa463267c9)

30000 points:
![image](https://github.com/user-attachments/assets/f2106315-4608-4d58-883e-464bb294c448)


Dependencies are OpenGL, GLAD, and GLFW.

I built this project in Visual Studio 2022, but I haven't quite figured out a workflow for interop'ing VS with github, without including a bunch of needless files.  For now, pasting source (excluding dependencies) will have to do for the Git repo.  Maybe in the future I'll find something I'm happy with, or just set up VS Code with a MakeFile.

This code randomly generates a set of points, finds their convex hull, and renders it to the screen.  Points within the convex hull are colored green (connected by a green line), while points outside of the convex hull are colored white.  The "red" points are the two points within the set that are the furthest away from each other.  Without an algorithm to find the convex hull, this would be an O(n^2) calculation, brute forcing all possible "furthest points".  Finding the convex hull with Graham Scan ends up being O(nlogn), limited by the sorting algorithm (I implemented it as an iterative merge sort, with a custom compare on the negative reciprocal slope as a proxy for polar angle).

Notably, various arrays for all the points are stack allocated.  Could make them heap allocated with a restructuring of the code, but I decided that stack allocated did the job, as I'm able to run the algorithm on 30000 points locally.

Using std::chrono, I profiled some hot spots as I was implementing the code, and made fixes accordingly (probably most notably, I changed from a selection sort to a merge sort for a massive boost).  The inefficient code is no longer present, but I left the profiling in.




With 30,000 points, comparing two approaches for finding the furthest points:

* Finding furthest points with the convex hull alg: 14926us (calculating the convex hull) + 9us (Finding furthest points from the convex hull subset)

* Finding furthest points with a "brute force" alg: 2889008us (iterating through all possible pairs of points)

Finding the convex hull before trying to find the furthest points is nearly ~200x faster.  As the number of points scales upwards, the difference will only become greater.
