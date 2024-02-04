// Copyright 2020-2022 David Colson. All rights reserved.

#include "rect_packing.h"

#include <defer.h>
#include <sort.h>
#include <resizable_array.inl>

namespace Packing {

// ***********************************************************************

struct SortByHeight {
    bool operator()(const Rect& a, const Rect& b) {
        return a.h < b.h;
    }
};

// ***********************************************************************

struct SortToOriginalOrder {
    bool operator()(const Rect& a, const Rect& b) {
        return a.ordering > b.ordering;
    }
};

// ***********************************************************************

void Packing::RowPackRects(ResizableArray<Rect>& rects, i32 width, i32 height) {
    for (usize i = 0; i < rects.count; i++) {
        rects[i].ordering = (int)i;
    }

    Sort(rects.pData, rects.count, SortByHeight());

    i32 xPos = 0;
    i32 yPos = 0;

    i32 largestHThisRow = 0;

    // Pack from left to right on a row
    i32 maxX = 0;
    i32 maxY = 0;
    i32 totalArea = 0;

    for (Rect& rect : rects) {
        if ((xPos + rect.w) > width) {
            yPos += largestHThisRow;
            xPos = 0;
            largestHThisRow = 0;
        }

        if ((yPos + rect.h) > height)
            break;

        rect.x = xPos;
        rect.y = yPos;

        xPos += rect.w;

        if (rect.h > largestHThisRow)
            largestHThisRow = rect.h;

        rect.wasPacked = true;
    }

    Sort(rects.pData, rects.count, SortToOriginalOrder());
}

// ***********************************************************************

struct SkylineNode {
    i32 x, y, width;
};

i32 CanRectFit(ResizableArray<SkylineNode>& nodes, i32 atNode, i32 rectWidth, i32 rectHeight, i32 width, i32 height) {
    // See if there's space for this rect at node "atNode"

    i32 x = nodes[atNode].x;
    i32 y = nodes[atNode].y;
    if (x + rectWidth > width)  // Check we're not going off the end of the image
        return -1;


    // We're going to loop over all the nodes from atNode to however many this new rect "covers"
    // We want to find the highest rect underneath this rect to place it at.
    i32 remainingSpace = rectWidth;
    i32 i = atNode;
    while (remainingSpace > 0) {
        if (i == nodes.count)
            return -1;

        SkylineNode& node = nodes[i];

        if (node.y > y)
            y = node.y;

        if (y + rectHeight > height)
            return -1;  // of the edge of the image
        remainingSpace -= node.width;
        i++;
    }
    return y;
}

// ***********************************************************************

void Packing::SkylinePackRects(ResizableArray<Rect>& rects, i32 width, i32 height) {
    for (usize i = 0; i < rects.count; i++) {
        rects[i].ordering = (int)i;
    }

    // Sort by a heuristic
    Sort(rects.pData, rects.count, SortByHeight());

    i32 maxX = 0;
    i32 maxY = 0;
    i32 totalArea = 0;

    ResizableArray<SkylineNode> nodes;
    defer(nodes.Free());

    nodes.PushBack({ 0, 0, width });

    for (Rect& rect : rects) {
        i32 bestHeight = height;
        i32 bestWidth = width;
        i32 bestNode = -1;
        i32 bestX, bestY;
        // We're going to search for the best location for this rect along the skyline
        for (usize i = 0; i < nodes.count; i++) {
            SkylineNode& node = nodes[i];
            i32 highestY = CanRectFit(nodes, (i32)i, rect.w, rect.h, width, height);
            if (highestY != -1) {
                // Settling a tie here on best height by checking lowest width we can use up
                if (highestY + rect.h < bestHeight || (highestY + rect.h == bestHeight && node.width < bestWidth)) {
                    bestNode = (int)i;
                    bestWidth = node.width;
                    bestHeight = highestY + rect.h;
                    bestX = node.x;
                    bestY = highestY;
                }
            }
        }

        if (bestNode == -1)
            continue;  // We failed

        // Add the actual rect, changing the skyline

        // First add the new node
        SkylineNode newNode;
        newNode.width = rect.w;
        newNode.x = bestX;
        newNode.y = bestY + rect.h;
        nodes.Insert(bestNode, newNode);

        // Now we have to find all the nodes underneath that new skyline level and remove them
        for (int i = bestNode + 1; i < (int)nodes.count; i++) {
            SkylineNode& node = nodes[i];
            SkylineNode& prevNode = nodes[i - 1];
            // Check to see if the current node is underneath the previous node
            // Remember that i starts as the first node after we inserted, so the previous node is the one we inserted
            if (node.x < prevNode.x + prevNode.width) {
                // Draw a picture of this
                i32 amountToShrink = (prevNode.x + prevNode.width) - node.x;
                node.x += amountToShrink;
                node.width -= amountToShrink;

                if (node.width <= 0)  // We've reduced this nodes with so much it can be removed
                {
                    nodes.Erase(i);
                    i--;  // Move back since we've removed a node
                } else {
                    break;  // If we don't need to remove this node we've reached the extents of our new covering node
                }
            } else {
                break;  // Nothing being covered
            }
        }

        // Find any skyline nodes that are the same height and remove them
        for (int i = 0; i < (int)nodes.count - 1; i++) {
            if (nodes[i].y == nodes[i + 1].y) {
                nodes[i].width += nodes[i + 1].width;
                nodes.Erase(i + 1);
                i--;
            }
        }

        rect.x = bestX;
        rect.y = bestY;

        rect.wasPacked = true;
    }
    Sort(rects.pData, rects.count, SortToOriginalOrder());
}
}
