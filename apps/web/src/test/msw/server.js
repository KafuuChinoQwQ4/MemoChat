import { setupServer } from "msw/node";
import { handlers } from "./handlers";
/** MSW server for tests — covers all HTTP routes */
export const server = setupServer(...handlers);
