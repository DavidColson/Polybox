// Copyright 2020-2022 David Colson. All rights reserved.

#include "rect_packing.h"

#include <defer.h>
#include <sort.h>

#include <algorithm>
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
        return a.m_ordering > b.m_ordering;
    }
};

// ***********************************************************************

void Packing::RowPackRects(ResizableArray<Rect>& rects, int width, int height) {
    for (size_t i = 0; i < rects.m_count; i++) {
        rects[i].m_ordering = (int)i;
    }

    Sort(rects.m_pData, rects.m_count, SortByHeight());

    int xPos = 0;
    int yPos = 0;

    int largestHThisRow = 0;

    // Pack from left to right on a row
    int maxX = 0;
    int maxY = 0;
    int totalArea = 0;

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

        rect.m_wasPacked = true;
    }

    Sort(rects.m_pData, rects.m_count, SortToOriginalOrder());
}

// ***********************************************************************

struct SkylineNode {
    int x, y, width;
};

int CanRectFit(ResizableArray<SkylineNode>& nodes, int atNode, int rectWidth, int rectHeight, int width, int height) {
    // See if there's space for this rect at node "atNode"

    int x = nodes[atNode].x;
    int y = nodes[atNode].y;
    if (x + rectWidth > width)  // Check we're not going off the end of the image
        return -1;


    // We're going to loop over all the nodes from atNode to however many this new rect "covers"
    // We want to find the highest rect underneath this rect to place it at.
    int remainingSpace = rectWidth;
    int i = atNode;
    while (remainingSpace > 0) {
        if (i == nodes.m_count)
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

void Packing::SkylinePackRects(ResizableArray<Rect>& rects, int width, int height) {
    for (size_t i = 0; i < rects.m_count; i++) {
        rects[i].m_ordering = (int)i;
    }

    // Sort by a heuristic
    Sort(rects.m_pData, rects.m_count, SortByHeight());

    int maxX = 0;
    int maxY = 0;
    int totalArea = 0;

    ResizableArray<SkylineNode> nodes;
    defer(nodes.Free());

    nodes.PushBack({ 0, 0, width });

    for (Rect& rect : rects) {
        int bestHeight = height;
        int bestWidth = width;
        int bestNode = -1;
        int bestX, bestY;
        // We're going to search for the best location for this rect along the skyline
        for (size_t i = 0; i < nodes.m_count; i++) {
            SkylineNode& node = nodes[i];
            int highestY = CanRectFit(nodes, (int)i, rect.w, rect.h, width, height);
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
        for (int i = bestNode + 1; i < (int)nodes.m_count; i++) {
            SkylineNode& node = nodes[i];
            SkylineNode& prevNode = nodes[i - 1];
            // Check to see if the current node is underneath the previous node
            // Remember that i starts as the first node after we inserted, so the previous node is the one we inserted
            if (node.x < prevNode.x + prevNode.width) {
                // Draw a picture of this
                int amountToShrink = (prevNode.x + prevNode.width) - node.x;
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
        for (int i = 0; i < (int)nodes.m_count - 1; i++) {
            if (nodes[i].y == nodes[i + 1].y) {
                nodes[i].width += nodes[i + 1].width;
                nodes.Erase(i + 1);
                i--;
            }
        }

        rect.x = bestX;
        rect.y = bestY;

        rect.m_wasPacked = true;
    }
    Sort(rects.m_pData, rects.m_count, SortToOriginalOrder());
}
}
