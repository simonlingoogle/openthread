#
#   Copyright (c) 2020 Google LLC.
#   All rights reserved.
#
#   This document is the property of Google LLC, Inc. It is
#   considered proprietary and confidential information.
#
#   This document may not be reproduced or transmitted in any form,
#   in whole or in part, without the express written permission of
#   Google LLC.
#

# Get a list of the available products and output as a list to the 'arg_products' argument
function(terbium_get_products arg_products)
    set(result "none")
    set(products_dir "${PROJECT_SOURCE_DIR}/src/products")
    file(GLOB products RELATIVE "${products_dir}" "${products_dir}/*")
    foreach(product IN LISTS products)
        if(IS_DIRECTORY "${products_dir}/${product}")
            list(APPEND result "${product}")
        endif()
    endforeach()

    set(${arg_products} "${result}" PARENT_SCOPE)
endfunction()
