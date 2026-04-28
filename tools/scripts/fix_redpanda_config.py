import yaml

with open('/etc/redpanda/redpanda.yaml', 'r') as f:
    cfg = yaml.safe_load(f)

cfg['advertised_kafka_api'][0]['address'] = '127.0.0.1'
cfg['advertised_pandaproxy_api'][0]['address'] = '127.0.0.1'

with open('/etc/redpanda/redpanda.yaml', 'w') as f:
    yaml.dump(cfg, f, default_flow_style=False)

print("done")
