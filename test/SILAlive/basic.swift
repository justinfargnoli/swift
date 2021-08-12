// RUN: %target-swift-frontend -emit-silgen %s -o %t | %target-sil-opt \
// RUN: --translation-validation --translation-validation %t 

let x = 1 + 1