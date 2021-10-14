const toggleClass = (el, name) => {
    if (el.classList.contains(name)) el.classList.remove(name)
    else el.classList.add(name)
    return el
}

export {toggleClass}
