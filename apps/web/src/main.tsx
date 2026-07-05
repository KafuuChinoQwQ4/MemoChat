import React from "react"
import ReactDOM from "react-dom/client"
import App from "./App"
import "@/theme/globals.css"

// Apply blur capability class once, before first render
if (!CSS.supports("backdrop-filter", "blur(1px)") &&
    !CSS.supports("-webkit-backdrop-filter", "blur(1px)")) {
  document.documentElement.classList.add("no-blur")
}

const root = document.getElementById("root")
if (!root) throw new Error("#root element not found")

ReactDOM.createRoot(root).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
)
