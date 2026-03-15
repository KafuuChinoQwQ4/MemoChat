# Values Layout

- `../values.yaml`: base shared defaults for all environments
- `dev.yaml`: development overrides
- `staging.yaml`: staging overrides
- `prod.yaml`: production overrides

Examples:

```bash
helm upgrade --install memochat deploy/kubernetes/charts/memochat \
  -f deploy/kubernetes/charts/memochat/values.yaml \
  -f deploy/kubernetes/charts/memochat/values/prod.yaml
```
