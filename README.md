# small_vector

A growable, contiguous container with user-controllable SBO. Modern C++, dependency-free.

## Motivation
Manual control over SBO size allows for fine-tuning of every usage separately in the codebase. It should be a compromise between the percentage of allocation cases and stack size of the vector objects.

For example, a 3d model might default to storing at most two materials, while a human-readable name of a tree node might assume 16 characters of storage to cover "most cases". Only when these defaults are exceeded an allocation will occur, which ideally will be rare enough. 

## Usage

Initialize `small_vector<T, I, Size>`, where:
* `T` is item type
* `I` is index type
* `Size` is number of elements in the SBO

The interface mimics a simplified `std::vector`. 

Type `I` used for size and capacity can be customized to minimize the size of small vectors. Total size of the structure will be `2 * sizeof(I) + Size * sizeof(T)`, and align of `T` will be propagated.