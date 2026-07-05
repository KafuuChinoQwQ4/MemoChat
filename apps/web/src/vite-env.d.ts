/// <reference types="vite/client" />

// Declare CSS module types
declare module "*.module.css" {
  const classes: Record<string, string>
  export default classes
}
